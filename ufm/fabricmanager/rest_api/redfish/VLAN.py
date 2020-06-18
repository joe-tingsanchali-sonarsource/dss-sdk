# External imports
import traceback

# Flask imports
from flask_restful import reqparse, Api, Resource

# Internal imports
import config
from rest_api.redfish.templates.VLAN import get_vlan_instance
from rest_api.redfish import redfish_constants

from common.ufmdb.redfish.redfish_vlan_backend import RedfishVlanBackend, RedfishVlanCollectionBackend

members = {}


class VlanAPI(Resource):

    def get(self, fab_id, sw_id, vlan_id):
        # """
        # HTTP GET
        # """
        try:
            redfish_backend = RedfishVlanBackend()
            response = redfish_backend.get(fab_id, sw_id, vlan_id)
        except Exception as e:
            self.log.exception(e)
            response = {"Status": redfish_Constants.SERVER_ERROR,
                        "Message": "Internal Server Error"}

        return response

    def post(self):
        raise NotImplementedError




class VlanCollectionAPI(Resource):

    def get(self, fab_id, sw_id):
        # """
        # HTTP GET
        # """
        try:
            redfish_backend = RedfishVlanCollectionBackend()
            response = redfish_backend.get(fab_id, sw_id)
        except Exception as e:
            self.log.exception(e)
            response = {"Status": redfish_Constants.SERVER_ERROR,
                        "Message": "Internal Server Error"}

        return response

    def post(self):
        raise NotImplementedError







class VlanEmulationAPI(Resource):
    """
    VLAN
    """

    def __init__(self, **kwargs):
        pass

    def get(self, ident1, ident2, ident3):
        # """
        # HTTP GET
        # """
        if ident1 not in members:
            return 'Client Error: Not Found', redfish_constants.NOT_FOUND
        if ident2 not in members[ident1]:
            return 'Client Error: Not Found', redfish_constants.NOT_FOUND
        if ident3 not in members[ident1][ident2]:
            return 'Client Error: Not Found', redfish_constants.NOT_FOUND

        return members[ident1][ident2][ident3], redfish_constants.SUCCESS


class VlanCollectionEmulationAPI(Resource):
    """
    VLAN Collection
    """

    def __init__(self, rest_base, suffix):
        self.cfg = {
            '@odata.context': rest_base + '$metadata#VLANCollection.VLANCollection',
            '@odata.id': rest_base + suffix,
            '@odata.type': '#VLANCollection.VLANCollection',
            'Description': 'Collection of VLANs',
            'Name': 'VLANs Collection'
        }

    def get(self, ident1, ident2):
        """
        HTTP GET
        """
        try:
            if ident1 not in members:
                return redfish_constants.NOT_FOUND
            if ident2 not in members[ident1]:
                return redfish_constants.NOT_FOUND
            vlans = []
            for vlan in members[ident1].get(ident2, {}).values():
                vlans.append({'@odata.id': vlan['@odata.id']})
            self.cfg['@odata.id'] = self.cfg['@odata.id'] + \
                '/' + ident1 + '/Switches/' + ident2 + '/VLANs'
            self.cfg['Members'] = vlans
            self.cfg['Members@odata.count'] = len(vlans)
            response = self.cfg, redfish_constants.SUCCESS
        except Exception:
            traceback.print_exc()
            response = redfish_constants.SERVER_ERROR

        return response


def CreateVlanEmulation(**kwargs):
    fab_id = kwargs['fab_id']
    switch_id = kwargs['switch_id']
    vlan_id = kwargs['vlan_id']
    if fab_id not in members:
        members[fab_id] = {}
    if switch_id not in members[fab_id]:
        members[fab_id][switch_id] = {}
    members[fab_id][switch_id][vlan_id] = get_vlan_instance(kwargs)
