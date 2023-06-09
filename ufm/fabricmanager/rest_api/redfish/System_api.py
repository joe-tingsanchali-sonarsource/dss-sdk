# The Clear BSD License
#
# Copyright (c) 2022 Samsung Electronics Co., Ltd.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted (subject to the limitations in the disclaimer
# below) provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of Samsung Electronics Co., Ltd. nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
# NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
# THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
# NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# External imports
import copy
import traceback

# Flask imports
from flask import request
# from flask import Flask
# from flask import make_response
# from flask import render_template

from flask_restful import Resource
# from flask_restful import reqparse
# from flask_restful import Api

from os.path import basename

from common.ufmdb.redfish.redfish_ufmdb import RedfishUfmdb
from common.ufmlog import ufmlog

# from common.ufmdb.redfish import ufmdb_redfish_resource
from common.ufmdb.redfish.redfish_system_backend import RedfishSystemBackend
# from common.ufmdb.redfish.redfish_system_backend import RedfishSystemCollectionBackend
from common.ufmdb.redfish.redfish_system_backend import RedfishCollectionBackend

# Internal imports
from rest_api.redfish.redfish_error_response import RedfishErrorResponse
from rest_api.redfish.templates.System import get_System_instance
from rest_api.redfish import redfish_constants
import config

members = {}


class UfmdbSystemAPI(Resource):
    """
    System UFMDB API
    """
    def __init__(self, **kwargs):
        self.rfdb = RedfishUfmdb(auto_update=True)
        self.log = ufmlog.log(module="RFDB", mask=ufmlog.UFM_REDFISH_DB)
        self.log.detail("UfmdbSystemAPI started.")

    def __del__(self):
        self.log.detail("UfmdbSystemAPI stopped.")

    def post(self, path="", **kwargs):
        """
        HTTP POST
        """
        try:
            path = path.strip('/')
            path = "/" + path
            payload = request.get_json(force=True)
            response = self.rfdb.post(path, payload)

        except Exception as e:
            self.log.exception(e)
            # traceback.print_exc()
            response = {"Status": 500,
                        "Message": "Internal Server Error"}

        return response

    def get(self, path="", **kwargs):
        """
        HTTP GET
        """
        try:
            path = path.strip('/')
            path = "/" + path
            response = self.rfdb.get(path, "{}")
            # payload = request.get_json(force=True)
            # response = self.rfdb.get(path, payload)
            self.log.error("=" * 40)

        except Exception as e:
            self.log.exception(e)
            # traceback.print_exc()
            response = {"Status": 500,
                        "Message": "Internal Server Error"}

        return response


class SystemAPI(Resource):
    def __init__(self):
        self.rf_err_resp = RedfishErrorResponse()

    def get(self, ident):
        try:
            redfish_backend = RedfishSystemBackend.create_instance(ident)
            response = redfish_backend.get()
        except Exception as e:
            response = self.rf_err_resp.get_server_error_response(e)
        return response

    def post(self, ident):
        try:
            payload = request.get_json(force=True)
            redfish_backend = RedfishSystemBackend.create_instance(ident)
            response = redfish_backend.put(payload)
        except NotImplementedError:
            response = self.rf_err_resp.get_method_not_allowed_response('System: ' + ident)
        except Exception as e:
            response = self.rf_err_resp.get_server_error_response(e)
        return response


class SystemCollectionAPI(Resource):
    def __init__(self):
        self.collection = basename(request.path.strip('/'))
        self.rf_err_resp = RedfishErrorResponse()

    def get(self):
        try:
            collection_backend = RedfishCollectionBackend.create_instance(self.collection)
            response = collection_backend.get()
        except Exception as e:
            response = self.rf_err_resp.get_server_error_response(e)
        return response

    # Or we can just not define it and flask will return the same response
    def post(self):
        return self.rf_err_resp.get_method_not_allowed_response('Systems')


class SystemEmulationAPI(Resource):
    """
    System Singleton API
    """

    def __init__(self):
        pass

    def get(self, ident):
        """
        HTTP GET
        """
        try:
            response = redfish_constants.NOT_FOUND
            if ident in members:
                con = members[ident]
                response = con, redfish_constants.SUCCESS
        except Exception:
            traceback.print_exc()
            response = redfish_constants.SERVER_ERROR
        return response


class SystemCollectionEmulationAPI(Resource):
    """
    System Collection API
    """

    def __init__(self):
        self.rb = config.rest_base
        self.config = {
            '@odata.context': self.rb + '$metadata#ComputerSystemCollection.ComputerSystemCollection',
            '@odata.id': self.rb + 'Systems',
            '@odata.type': '#ComputerSystemCollection.ComputerSystemCollection',
            'Description': 'Collection of Computer Systems',
            'Members@odata.count': len(members),
            'Name': 'System Collection'
        }
        self.config['Members'] = [
            {'@odata.id': item['@odata.id']} for item in list(members.values())]

    def get(self):
        """
        HTTP GET
        """
        try:
            response = self.config, redfish_constants.SUCCESS
        except Exception:
            traceback.print_exc()
            response = redfish_constants.SERVER_ERROR

        return response


class CreateSystemEmulation(Resource):
    """
    CreateSystem
    """

    def __init__(self, **kwargs):
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])

    def put(self, ident, nqn, ns):
        try:
            global cfg
            global wildcards
            wildcards['sys_id'] = ident
            wildcards['nqn_id'] = nqn
            wildcards['ns_id'] = ns
            cfg = get_System_instance(wildcards)
            members[ident] = cfg
            response = cfg, redfish_constants.SUCCESS

        except Exception:
            traceback.print_exc()
            response = redfish_constants.SERVER_ERROR

        return response
