import os
import re
import subprocess
import threading
import time
import uuid

from utils.log_setup import get_logger
from utils.utils import find_process_pid


class MinioStats(object):
    def __init__(self, cluster_id, target_id, ustat_path='/usr/dss/nkv-target/bin/ustat', statsdb_obj=None):
        """
        :param cluster_id: Cluster ID
        :param target_id:  Target ID
        :param ustat_path: path to ustat binary
        :param statsdb_obj: Metrics DB object instance to push the stats as a message which will
                          eventually push to metrics DB
        """
        self.log = get_logger()
        self.event = threading.Event()
        self.cluster_id = cluster_id
        self.target_id = target_id
        self.ustat_path = ustat_path
        self.device_subsystem_map = {}
        self.statsdb_obj = statsdb_obj

    def get_device_subsystem_map(self):
        root_dir = '/sys/devices/virtual/nvme-subsystem'
        if not os.access(root_dir, os.X_OK):
            return
        for root, dirs, files in os.walk(root_dir):
            if not root.startswith(os.path.join(root_dir, 'nvme-subsys')):
                continue
            for d in dirs:
                if d.startswith('nvme'):
                    subsys_path = os.path.join(root, d)
                    with open(subsys_path + '/subsysnqn') as f:
                        nqn = f.readline().strip()
                    with open(subsys_path + '/serial') as f:
                        serial = f.readline().strip()
                    self.device_subsystem_map[d + 'n1'] = {'nqn': nqn, 'serial': serial}

                    """
                    with open(subsys_path + '/address') as f:
                        out = f.readline().strip()
                    with open(subsys_path + '/transport') as f:
                        transport = f.readline().strip()
                    """
        self.log.info('Device subsystem map - %s', str(self.device_subsystem_map))

    @staticmethod
    def _convert_stats_dict_to_json_dict(stats_dict):
        json_dict = dict()
        for k, v in stats_dict.items():
            keys = k.split('.')
            json_kv = json_dict
            for key in keys[:-1]:
                json_kv = json_kv.setdefault(key, {})
            json_kv[keys[-1]] = v
        return json_dict

    def _convert_nkv_disk_metrics_to_tuples(self, nkv_drive_io_dict, minio_uuid, timestamp, dsk_id, cpu_id=None):
        tuples = []
        for counter in nkv_drive_io_dict:
            val = nkv_drive_io_dict[counter]
            if cpu_id:
                metric_path = "cluster_id_%s.target_id_%s.minio_id_%s.disk_id_%s.cpu_id_%s.io.%s" % \
                              (self.cluster_id, self.target_id, minio_uuid,
                               dsk_id, cpu_id, counter)
                tuples.append((metric_path, (timestamp, val)))
            else:
                metric_path = "cluster_id_%s.target_id_%s.minio_id_%s.disk_id_%s.io.%s" % \
                              (self.cluster_id, self.target_id, minio_uuid,
                               dsk_id, counter)
                tuples.append((metric_path, (timestamp, val)))
        return tuples

    def poll_statistics(self, pid, stopper_event, sec):
        minio_uuid = None
        proc_file = os.path.join('/proc', str(pid), 'cmdline')
        proc_cmd = 'minio'
        with open(proc_file) as f:
            proc_cmd = f.readline()
        minio_uuid = uuid.uuid3(uuid.NAMESPACE_DNS, proc_cmd)

        cmd = self.ustat_path + ' -p ' + str(pid) + ' 1 1'
        while not stopper_event.is_set():
            pipe = subprocess.Popen(cmd.split(),
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
            out, err = pipe.communicate()
            out_list = []
            if err:
                self.log.error('Error in running cmd %s -  %s', cmd, err)
            if out:
                self.log.debug('cmd %s output <%s>', cmd, out)
                out_list = out.split('\n')

            stats_output = {}
            for line in out_list:
                if not line:
                    continue
                try:
                    k, v = line.split('=')
                    stats_output[k] = v
                except Exception:
                    self.log.exception('Failed to handle line %s', line)
            if stats_output:
                # Convert the flat key/value to json dict
                stats_json_dict = self._convert_stats_dict_to_json_dict(stats_output)
                nkv_stats_json_dict = stats_json_dict['nkv']['device']
                self.log.debug('STATS JSON %s', stats_json_dict)
                timestamp = time.time()
                tuples = []

                for dsk in nkv_stats_json_dict:
                    if dsk.startswith('nvme'):
                        if dsk not in self.device_subsystem_map:
                            continue
                        # subsys_name = self.device_subsystem_map[dsk]['nqn']
                        subsys_serial = self.device_subsystem_map[dsk]['serial']
                        nkv_drive_dict = nkv_stats_json_dict[dsk]
                        for k in nkv_drive_dict:
                            if k == 'cpu':
                                nkv_drive_cpu_dict = nkv_drive_dict['cpu']
                                for cpu in nkv_drive_cpu_dict:
                                    cpu_id = cpu
                                    nkv_drive_io_dict = nkv_drive_cpu_dict[cpu]['io']['outstanding']
                                    tuples += self._convert_nkv_disk_metrics_to_tuples(nkv_drive_io_dict, minio_uuid,
                                                                                       timestamp, subsys_serial, cpu_id)
                            elif k == 'io':
                                nkv_drive_io_dict = nkv_drive_dict['io']['outstanding']
                                tuples += self._convert_nkv_disk_metrics_to_tuples(nkv_drive_io_dict, minio_uuid,
                                                                          timestamp, subsys_serial)

                # Push all the metrics other than nkv.device.nvme into graphite DB AS-IS
                for k, v in stats_output.iteritems():
                    if not k.startswith('nkv.device.nvme'):
                        metric_path = "cluster_id_%s.target_id_%s.minio_id_%s.%s" % \
                                      (self.cluster_id, self.target_id, minio_uuid, k)
                        tuples.append((metric_path, (timestamp, v)))

                if tuples:
                    self.log.debug('Metrics sent to DB %s', str(tuples))

                    if self.statsdb_obj:
                        self.statsdb_obj.submit_message(tuples)
            time.sleep(sec)

        self.log.info('Minio Poll statistics thread exited')

    def get_minio_instances(self):
        proc_name = re.compile(r"^minio$")
        cmds = ["minio"]
        pid_list = None
        try:
            pid_list = find_process_pid(proc_name, cmds)
            self.log.info('Minio instance PIDs %s', str(pid_list))
        except:
            self.log.exception('Error in getting the PID list for MINIO instances')
        return pid_list

    def run_stats_collector(self):
        self.event.clear()
        pid_list = self.get_minio_instances()
        for pid in pid_list:
            self.log.info('Starting minio stats collection thread for pid %s', str(pid))
            thr = threading.Thread(target=self.poll_statistics, args=[pid, self.event, 2])
            thr.start()
            self.log.info('Started minio stats collection thread for pid %s', str(pid))

    def stop_stats_collector(self):
        self.log.info('Stopping the minio stats collection')
        self.event.set()


if __name__ == '__main__':
    minio_statsdb_obj = MinioStats('123', '456')
    minio_statsdb_obj.get_device_subsystem_map()
    print(minio_statsdb_obj.device_subsystem_map)
    print(minio_statsdb_obj.get_minio_instances())
    minio_statsdb_obj.run_stats_collector()
    time.sleep(10)
    minio_statsdb_obj.stop_stats_collector()
    time.sleep(20)
