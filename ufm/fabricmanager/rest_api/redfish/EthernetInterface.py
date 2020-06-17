# External imports
import traceback

# Flask imports
from flask import request
from flask_restful import reqparse, Api, Resource

# Internal imports
import config
from common.ufmlog import ufmlog
from common.ufmdb.redfish.redfish_ethernet_backend import RedfishEthernetBackend, RedfishEthernetCollectionBackend
from .redfish_error_response import RedfishErrorResponse
from .templates.EthernetInterface import get_ethernet_interface_instance
from rest_api.redfish import redfish_constants

members = {}


class EthernetInterfaceAPI(Resource):
    def __init__(self):
        self.rf_err_resp = RedfishErrorResponse()

    def get(self, sys_id, eth_id):
        try:
            redfish_backend = RedfishEthernetBackend.create_instance(
                sys_id, eth_id)
            response = redfish_backend.get()
        except Exception as e:
            response = self.rf_err_resp.get_server_error_response(e)
        return response

    def post(self, sys_id, eth_id):
        try:
            payload = request.get_json(force=True)
            redfish_backend = RedfishEthernetBackend.create_instance(
                sys_id, eth_id)
            response = redfish_backend.put(payload)
        except NotImplementedError as e:
            response = self.rf_err_resp.get_method_not_allowed_response('EthernetInterface: ' + eth_id)
        except Exception as e:
            response = self.rf_err_resp.get_server_error_response(e)
        return response


class EthernetInterfaceCollectionAPI(Resource):
    def __init__(self):
        self.rf_err_resp = RedfishErrorResponse()

    def get(self, sys_id):
        try:
            redfish_backend = RedfishEthernetCollectionBackend.create_instance(
                sys_id)
            response = redfish_backend.get()
        except Exception as e:
            response = self.rf_err_resp.get_server_error_response(e)
        return response

    def post(self):
        return self.rf_err_resp.get_method_not_allowed_response('EthernetInterfaces')


class EthernetInterfaceEmulationAPI(Resource):
    """
    EthernetInterface
    """

    def __init__(self, **kwargs):
        pass

    def get(self, ident1, ident2):
        # """
        # HTTP GET
        # """
        if ident1 not in members:
            return 'Client Error: Not Found', redfish_constants.NOT_FOUND
        if ident2 not in members[ident1]:
            return 'Client Error: Not Found', redfish_constants.NOT_FOUND

        return members[ident1][ident2], redfish_constants.SUCCESS


class EthernetInterfaceCollectionEmulationAPI(Resource):
    """
    EthernetInterface Collection
    """

    def __init__(self, rest_base, suffix):
        self.cfg = {
            '@odata.context': rest_base + '$metadata#EthernetInterfaceCollection.EthernetInterfaceCollection',
            '@odata.id': rest_base + suffix,
            '@odata.type': '#EthernetInterfaceCollection.EthernetInterfaceCollection',
            'Description': 'Collection of Ethernet Interfaces',
            'Name': 'Ethernet interfaces Collection'
        }

    def get(self, ident):
        """
        HTTP GET
        """
        try:
            if ident not in members:
                return redfish_constants.NOT_FOUND
            ethernet_interface = []
            for nic in members.get(ident, {}).values():
                ethernet_interface.append({'@odata.id': nic['@odata.id']})
            self.cfg['@odata.id'] = self.cfg['@odata.id'] + \
                '/' + ident + '/EthernetInterfaces'
            self.cfg['Members'] = ethernet_interface
            self.cfg['Members@odata.count'] = len(ethernet_interface)
            response = self.cfg, redfish_constants.SUCCESS
        except Exception:
            traceback.print_exc()
            response = redfish_constants.SERVER_ERROR

        return response


def CreateEthernetInterfaceEmulation(**kwargs):
    sys_id = kwargs['sys_id']
    nic_id = kwargs['nic_id']
    if sys_id not in members:
        members[sys_id] = {}
    members[sys_id][nic_id] = get_ethernet_interface_instance(kwargs)
