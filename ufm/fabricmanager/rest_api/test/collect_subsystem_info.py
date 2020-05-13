import redfish_client
import argparse
import json
import socket

'''
Create a connection to the UFM redfish API service using the redfish-client
package.
redfish_client abstracts the connection and operations to a redfish compliant
service.
The connect call returns the redfish service root.
Client code can then iterate through redfish endpoints hierarchy without
making any explicit rest calls.
'''

def getSystemInfo(service_addr):
    # Will throw exception if connection fails. No need to catch it as nothing more to do
    # in that case
    root = redfish_client.connect(service_addr, '', '')

    # If the Systems collection is empty return an empty dictionary
    if not root.Systems.Members:
        return {}

    # Add system info
    system_info_dict = {}
    tgt_cnt = 0

    for system in root.Systems.Members:
        # Add nqn for any subsystems
        if system.Identifiers:
            for identifier in system.Identifiers:
                if identifier.DurableNameFormat == 'NQN':
                    tgt_cnt += 1
                    tgt_str = 'System ' + str(tgt_cnt)
                    system_info_dict[tgt_str] = {}
                    system_info_dict[tgt_str]['NQN'] = identifier.DurableName
                    system_info_dict[tgt_str]['Target_Server_Name'] = system.oem['ServerName']

                    #Add Network Information
                    if system.EthernetInterfaces:
                        # No transports to add since no NICs available
                        if not system.EthernetInterfaces.Members:
                            continue
                        # Add the transport fields for each NIC
                        ethernet_list = []
                        ethernet_str = ','.join(tuple(('{:^20}'.format("Mac Addr"),
                                                       '{:^14}'.format("IP Addr"),
                                                       '{:^6}'.format("Port"),
                                                       '{:^12}'.format("Trans type"),
                                                       '{:^10}'.format("Status"))))
                        ethernet_list.append(ethernet_str)
                        ethernet_str = ','.join(tuple(('{:^20}'.format(''.rjust(20,'-')),
                                                       '{:^14}'.format(''.rjust(14,'-')),
                                                       '{:^6}'.format(''.rjust(6,'-')),
                                                       '{:^12}'.format(''.rjust(12,'-')),
                                                       '{:^10}'.format(''.rjust(10,'-')))))
                        ethernet_list.append(ethernet_str)
                        for interface in system.EthernetInterfaces.Members:
                            if interface.IPv4Addresses:
                                for ipv4addr in interface.IPv4Addresses:
                                    if ipv4addr.Address:
                                        ethernet_str = ','.join(tuple((
                                            '{:^20}'.format(str(interface.MACAddress)),
                                            '{:^14}'.format(str(ipv4addr.Address)),
                                            '{:^6}'.format(str(ipv4addr.oem.Port)),
                                            '{:^12}'.format(str(ipv4addr.oem.SupportedProtocol)),
                                            '{:^10}'.format(str(interface.LinkStatus)))))
                                        ethernet_list.append(ethernet_str)
                            if interface.IPv6Addresses:
                                for ipv6addr in interface.IPv6Addresses:
                                    if ipv6addr.Address:
                                        ethernet_str = ','.join(tuple((
                                            '{:^20}'.format(str(interface.MACAddress)),
                                            '{:^14}'.format(str(ipv6addr.Address)),
                                            '{:^6}'.format(str(ipv4addr.oem.Port)),
                                            '{:^12}'.format(str(ipv4addr.oem.SupportedProtocol)),
                                            '{:^10}'.format(str(interface.LinkStatus)))))
                                        ethernet_list.append(ethernet_str)

                            system_info_dict[tgt_str]['EthernetInterfaces'] = ethernet_list

                    #Add storage Information
                    if system.Storage:
                        # No storage drives to add
                        if not system.Storage.Members:
                            continue
                        # Add the storage fields for each drive
                        storage_list = []
                        storage_str = ','.join(tuple(('{:^18}'.format("Serial Number"),
                                                       '{:^18}'.format("MediaType"),
                                                       '{:^18}'.format("Manufacturer"),
                                                       '{:^24}'.format("Model"),
                                                       '{:^18}'.format("FW Revision"))))
                        storage_list.append(storage_str)
                        storage_str = ','.join(tuple(('{:^18}'.format(''.rjust(18,'-')),
                                                       '{:^18}'.format(''.rjust(18,'-')),
                                                       '{:^18}'.format(''.rjust(18,'-')),
                                                       '{:^24}'.format(''.rjust(24,'-')),
                                                       '{:^18}'.format(''.rjust(18,'-')))))
                        storage_list.append(storage_str)
                        for storage in system.Storage.Members:
                            if storage.Drives:
                                for drive in storage.Drives:
                                    storage_str = ','.join(tuple(('{:^18}'.format(str(drive.SerialNumber)),
                                                                   '{:^18}'.format(str(drive.MediaType)),
                                                                   '{:^18}'.format(str(drive.Manufacturer)),
                                                                   '{:^24}'.format(str(drive.Model)),
                                                                   '{:^18}'.format(str(drive.Revision)))))
                                    storage_list.append(storage_str)
                    system_info_dict[tgt_str]['Storage'] = storage_list
    return system_info_dict

if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='Process Server\'s Configuration Settings.')
    parser.add_argument("--host", help="IP Address or FQDN of Server",
                        dest="host", required=True)
    parser.add_argument("--port", help="Port of Server",
                        dest="port", default=5000)

    args = parser.parse_args()

    service_addr = 'http://' + args.host + ':' + str(args.port)

    systemInfo = {}
    systemInfo = getSystemInfo(service_addr)
    print('System Information : \n', json.dumps(systemInfo, indent=2))