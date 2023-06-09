# redfish_responses.py
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


redfish_responses = {
    '1': {
        "@odata.context": "/redfish/v1/$metadata#ServiceRoot.ServiceRoot",
        "@odata.type": "#ServiceRoot.v1_0_0.ServiceRoot",
        "@odata.id": "/redfish/v1",
        "Id": "RootService",
        "Name": "Root Service",
        "ProtocolFeaturesSupported": {
            "ExpandQuery": {
                "ExpandAll": False
            },
            "SelectQuery": False
        },
        "RedfishVersion": "1.8.0",
        "UUID": "None",
        "Vendor": "Samsung",
        "JSONSchemas": {
            "@odata.id": "/redfish/v1/JSONSchemas"
        },
        "Systems": {
            "@odata.id": "/redfish/v1/Systems"
        },
        "Managers": {
            "@odata.id": "/redfish/v1/Managers"
        },
        "Fabrics": {
            "@odata.id": "/redfish/v1/Fabrics"
        },
    },
    '1.1': {
        "@odata.context": "/redfish/v1/$metadata#ComputerSystemCollection.ComputerSystemCollection",
        "@odata.type": "#ComputerSystemCollection.ComputerSystemCollection",
        "@odata.id": "",
        "Description": "Collection of Computer Systems",
        "Members": list(),
        "Members@odata.count": 0,
        "Name": "System Collection"
    },
    '1.2': {
        "@odata.id": "/redfish/v1/JSONSchemas",
        "id": "http://redfish.dmtf.org/schemas/v1/redfish-schema.v1_2_0",
        "type": "object",
        "$schema": "http://redfish.dmtf.org/schemas/v1/redfish-schema.v1_2_0",
        "title": "Redfish Schema Extension",
        "description": "The properties defined in this schema shall adhere to the requirements "
                       + "of the Redfish Specification and the semantics of the descriptions in this file.",
        "allOf": [
            {
                "$ref": "http://json-schema.org/draft-04/schema"
            }
        ],
        "definitions": {
            "readonly": {
                "type": "boolean",
                "description": "This property shall designate a property to be readonly when set to true."
            },
            "requiredOnCreate": {
                "type": "array",
                "items": {
                    "type": "boolean"
                },
                "description": "This property is required to be specified in the body "
                               + "of a POST request to create the resource."
            },
            "longDescription": {
                "type": "string",
                "description": "This attribute shall contain normative language relating "
                               + "to the Redfish Specification and documentation."
            },
            "copyright": {
                "type": "string",
                "description": "This attribute shall contain the copyright notice for the schema."
            },
            "deprecated": {
                "type": "string",
                "description": "The term shall be applied to a property in order to specify "
                               + "that the property is deprecated.  The value of the string should "
                               + "explain the deprecation, including new property or properties to be "
                               + "used. The property can be supported in new and existing implementations, "
                               + "but usage in new implementations is discouraged.  "
                               + "Deprecated properties are likely to be removed in a future major version "
                               + "of the schema."
            },
            "enumDescriptions": {
                "type": "object",
                "description": "This attribute shall contain informative language related to the "
                               + "enumeration values of the property."
            },
            "enumLongDescriptions": {
                "type": "object",
                "description": "This attribute shall contain normative language relating to the "
                               + "enumeration values of the property."
            },
            "enumDeprecated": {
                "type": "object",
                "description": "The term shall be applied to a value in order to specify that the "
                               + "value is deprecated.  The value of the string should explain the "
                               + "deprecation, including new value to be used.  The value can be supported "
                               + "in new and existing implementations, but usage in new implementations is "
                               + "discouraged.  Deprecated values are likely to be removed in a future major "
                               + "version of the schema."
            },
            "units": {
                "type": "string",
                "description": "This attribute shall contain the units of measure used by the value of the property."
            }
        },
        "properties": {
            "readonly": {
                "$ref": "#/definitions/readonly"
            },
            "longDescription": {
                "$ref": "#/definitions/longDescription"
            },
            "copyright": {
                "$ref": "#/definitions/copyright"
            },
            "enumDescriptions": {
                "$ref": "#/definitions/enumDescriptions"
            },
            "enumLongDescriptions": {
                "$ref": "#/definitions/enumLongDescriptions"
            },
            "units": {
                "$ref": "#/definitions/units"
            }
        }
    },
    '1.3': {
        "@odata.context": "/redfish/v1/$metadata#ManagerCollection.ManagerCollection",
        "@odata.type": "#ManagerCollection.ManagerCollection",
        "@odata.id": "/redfish/v1/Managers",
        "Name": "Manager Collection",
        "Description": "Collection of Managers",
        "Members": [
            {
                "@odata.id": "/redfish/v1/Managers/ufm"
            }
        ],
        "Members@odata.count": 1,
    },
    '1.1.1': {
        "@odata.context": "/redfish/v1/$metadata#ComputerSystem.ComputerSystem",
        "@odata.type": "#ComputerSystem.v1_9_0.ComputerSystem",
        "@odata.id": "",
        "Description": "System containing subsystem(s)",
        "Id": "",
        "Identifiers": [
            {
                "DurableName": "",
                "DurableNameFormat": "UUID"
            }
        ],
        "IPv4Addresses": [
            {
                "Address": "",
                "SubnetMask": "",
                "AddressOrigin": "",
                "Gateway": ""
            }
        ],
        "IPv6DefaultGateway": "",
        "IPv6Addresses": [
            {
                "Address": "",
                "PrefixLength": 0,
                "AddressOrigin": "",
                "AddressState": ""
            }
        ],
        "Links": {
            "SupplyingComputerSystems": list()
        }
    },
    '1.1.2': {
        "@odata.context": "/redfish/v1/$metadata#ComputerSystem.ComputerSystem",
        "@odata.id": "",
        "@odata.type": "#ComputerSystem.v1_9_0.ComputerSystem",
        "Description": "System representing the drive resources",
        "Id": "",
        "Identifiers": [
            {
                "DurableName": "",
                "DurableNameFormat": "NQN"
            }
        ],
        "Status": {
            "State": "Enabled",
            "Health": "OK"
        },
        "Storage": dict(),
        "EthernetInterfaces": dict(),

        "Name": "System",
        "oem": dict(),
        "Links": {"ConsumingComputerSystems": list()}
    },
    '1.1.2.1': {
        "@odata.context": "/redfish/v1/$metadata#StorageCollection.StorageCollection",
        "@odata.type": "#StorageCollection.StorageCollection",
        "@odata.id": "",
        "Description": "Collection of Storage information",
        "Members": list(),
        "Members@odata.count": 0,
        "Name": "Storage Collection",
        "oem": dict(),
    },
    '1.1.2.1.1': {
        "@odata.context": "/redfish/v1/$metadata#Storage.Storage",
        "@odata.type": "Storage.v1_8_0.Storage",
        "@odata.id": "",
        "Id": "",
        "Description": "Storage information",
        "Drives": list(),
        "Name": "Storage",
        "oem": dict()
    },
    '1.1.2.1.1.1': {
        "@odata.context": "/redfish/v1/$metadata#DriveCollection.DriveCollection",
        "@odata.id": "",
        "@odata.type": "#DriveCollection.DriveCollection",
        "Description": "Collection of drives",
        "Members": list(),
        "Members@odata.count": 0,
        "Name": "Drive Collection"
    },
    '1.1.2.1.1.1.1': {
        "@odata.context": "/redfish/v1/$metadata#Drive.Drive",
        "@odata.type": "Drive.v1_8_0.Drive",
        "@odata.id": "",
        "Description": "Drive Information",
        "Name": "Storage Drive Information",
        "BlockSizeBytes": 512,
        "CapacityBytes": 0,
        "Id": "",
        "Manufacturer": "",
        "MediaType": "",
        "Model": "",
        "Protocol": "",
        "Revision": "",
        "SerialNumber": "",
        "oem": dict()
    },
    '1.1.2.2': {
        "@odata.context": "/redfish/v1/$metadata#EthernetInterfaceCollection.EthernetInterfaceCollection",
        "@odata.type": "#EthernetInterfaceCollection.EthernetInterfaceCollection",
        "@odata.id": "",
        "Description": "Collection of Ethernet Interfaces",
        "Members": list(),
        "Members@odata.count": 0,
        "Name": "Ethernet interfaces Collection"
    },
    '1.1.2.2.1': {
        "@odata.context": "/redfish/v1/$metadata#EthernetInterface.EthernetInterface",
        "@odata.type": "#EthernetInterface.v1_5_1.EthernetInterface",
        "@odata.id": "",
        "Name": "Ethernet Interface",
        "Description": "Ethernet Interface information",
        "Id": "",
        "LinkStatus": "",
        "MACAddress": "",
        "SpeedMbps": 0,
        "IPv4Addresses": [
            {
                "Address": "",
                "SubnetMask": "",
                "AddressOrigin": "",
                "Gateway": "",
                "oem": dict({'Port': 0,
                             'SupportedProtocol': ""})
            }
        ],
        "IPv6DefaultGateway": "",
        "IPv6Addresses": [
            {
                "Address": "",
                "PrefixLength": 0,
                "AddressOrigin": "",
                "AddressState": "",
                "oem": dict({'Port': 0,
                             'SupportedProtocol': ""})
            }
        ]
    },
    '1.3.1': {
        "@odata.context": "/redfish/v1/$metadata#UFM.v1_0_0.UFM",
        "@odata.type": "##UFM.v1_0_0.UFM",
        "@odata.id": "/redfish/v1/Managers/ufm",
        "Id": "UFM",
        "Name": "UFM System Manager",
        "UUID": "",
        "LogServices": {
            "@odata.id": "/redfish/v1/Managers/ufm/LogServices",
        },
        "Actions": {
            "#Ufm.Reset": {
                "target": "/redfish/v1/Managers/ufm/Actions/Ufm.Reset",
                "ResetType@Redfish.AllowableValues": [
                    "ForceOff",
                    "ForceRestart"]
            },
        }
    },
    '1.3.1.1': {
        "@odata.context": "/redfish/v1/$metadata#LogServiceCollection.LogServiceCollection",
        "@odata.type": "#LogServiceCollection.LogServiceCollection",
        "@odata.id": "/redfish/v1/Managers/ufm/LogServices",
        "Name": "Log Service Collection",
        "Description": "Collection of Log Services for UFM Manager",
        "Members": [
            {
                "@odata.id": "/redfish/v1/Managers/ufm/LogServices/Log"
            }
        ],
        "Members@odata.count": 1,
    },
    '1.3.1.1.1': {
        "@odata.context": "/redfish/v1/$metadata#LogServices.v1_1_0.LogServices",
        "@odata.type": "#LogServices.v1_1_0.LogServices",
        "@odata.id": "/redfish/v1/Managers/ufm/LogServices/Log",
        "Id": "Log",
        "Name": "UFM Log Services",
        "MaxNumberOfRecords": 1000,
        "Oem": {},
        "OverWritePolicy": "WrapsWhenFull",
        "Actions": {
            "#LogService.ClearLog": {
                "target": "/redfish/v1/Managers/ufm/LogServices/Log/Actions/LogService.ClearLog",
            },
            "#LogService.Entries": {
                "target": "/redfish/v1/Managers/ufm/LogServices/Log/Actions/LogService.Entries",
            },
            "#LogService.GetMask": {
                "target": "/redfish/v1/Managers/ufm/LogServices/Log/Actions/LogService.GetMask",
                "@Redfish.ActionInfo": "/redfish/v1/Managers/ufm/LogServices/Log/GetMaskActionInfo"
            },
            "#LogService.SetMask": {
                "target": "/redfish/v1/Managers/ufm/LogServices/Log/Actions/LogService.SetMask",
                "@Redfish.ActionInfo": "/redfish/v1/Managers/ufm/LogServices/Log/SetMaskActionInfo"
            },
            "#LogService.GetRegistry": {
                "target": "/redfish/v1/Managers/ufm/LogServices/Log/Actions/LogService.GetRegistry",
            },
        }
    },
    '1.3.1.1.1.1': {
        "@odata.context": "/redfish/v1/$metadata#LogEntryCollection.LogEntryCollection",
        "@odata.type": "#LogEntryCollection.LogEntryCollection",
        "@odata.id": "/redfish/v1/Managers/ufm/LogServices/Log/Entries",
        "Name": "Log Service Collection",
        "Description": "Collection of Logs for this System",
        "Members": [],
        "Members@odata.count": 0,
    },
    '1.3.1.1.1.2': {
        "@odata.context": "/redfish/v1/$metadata#ActionInfo.v1_1_0.ActionInfo",
        "@odata.type": "#ActionInfo.v1_1_0.ActionInfo",
        "@odata.id": "/redfish/v1/Managers/ufm/LogServices/Log/SetMaskActionInfo",
        "Id": "SetMaskActionInfo",
        "Name": "Set Mask Action Info",
        "Parameters": [
            {
                "Name": "ErrorMask",
                "Required": False,
                "DataType": "number",
                "MinimumValue": 0,
                "MaximumValue": 0xFFFFFFFF
            }, {
                "Name": "WarningMask",
                "Required": False,
                "DataType": "number",
                "MinimumValue": 0,
                "MaximumValue": 0xFFFFFFFF
            }, {
                "Name": "InfoMask",
                "Required": False,
                "DataType": "number",
                "MinimumValue": 0,
                "MaximumValue": 0xFFFFFFFF
            }, {
                "Name": "DebugMask",
                "Required": False,
                "DataType": "number",
                "MinimumValue": 0,
                "MaximumValue": 0xFFFFFFFF
            }, {
                "Name": "DetailMask",
                "Required": False,
                "DataType": "number",
                "MinimumValue": 0,
                "MaximumValue": 0xFFFFFFFF},
        ]
    },
    '1.3.1.1.1.3': {
        "@odata.context": "/redfish/v1/$metadata#ActionInfo.v1_1_0.ActionInfo",
        "@odata.type": "#ActionInfo.v1_1_0.ActionInfo",
        "@odata.id": "/redfish/v1/Managers/ufm/LogServices/Log/GetMaskActionInfo",
        "Id": "GetMaskActionInfo",
        "Name": "Get Mask Action Info",
        "Parameters": [{
            "Name": "MaskType",
            "Required": True,
            "DataType": "string",
            "AllowableValues": [
                "All",
                "ErrorMask",
                "WarningMask",
                "InfoMask",
                "DebugMask",
                "DetailMask"
            ],
        },
        ]
    },
    '1.4': {
        "@odata.context": "/redfish/v1/$metadata#FabricCollection.FabricCollection",
        "@odata.type": "#FabricCollection.FabricCollection",
        "@odata.id": "/redfish/v1/Fabrics",
        "Name": "Fabric Collection",
        "Description": "Collection of Fabrics",
        "Members": list(),
        "Members@odata.count": 0,
    },
    '1.4.1': {
        "@odata.context": "/redfish/v1/$metadata#Fabric.v1_1_1.Fabric",
        "@odata.type": "##Fabric.v1_1_1.Fabric",
        "@odata.id": "",
        "Id": "",
        "Name": "Fabric",
        "Switches": dict()
    },
    '1.4.1.1': {
        "@odata.context": "/redfish/v1/$metadata#SwitchCollection.SwitchCollection",
        "@odata.type": "#SwitchCollection.SwitchCollection",
        "@odata.id": "",
        "Name": "Switch Collection",
        "Description": "Collection of Switch information",
        "Members": list(),
        "Members@odata.count": 0,
    },
    '1.4.1.1.1': {
        "@odata.context": "/redfish/v1/$metadata#Switch.Switch",
        "@odata.type": "#Switch.v1_3_1.Switch",
        "@odata.id": "",
        "Id": "",
        "Name": "Switch",
        "Description": "Switch information",
        "Ports": dict(),
        "VLANs": dict(),
    },
    '1.4.1.1.1.1': {
        "@odata.context": "/redfish/v1/$metadata#PortCollection.PortCollection",
        "@odata.type": "#PortCollection.PortCollection",
        "@odata.id": "",
        "Name": "Port Collection",
        "Description": "Collection of Port information",
        "Memners": list(),
        "Memners@odata.count": 0,
    },
    '1.4.1.1.1.1.1': {
        "@odata.context": "/redfish/v1/$metadata#Port.Port",
        "@odata.type": "#Port.v1_2_1.Port",
        "@odata.id": "",
        "Id": "",
        "Name": "Port",
        "Description": "Port information",
    },
    '1.4.1.1.1.2': {
        "@odata.context": "/redfish/v1/$metadata#VLANCollection.VLANCollection",
        "@odata.type": "#VLANCollection.VLANCollection",
        "@odata.id": "",
        "Name": "VLAN Collection",
        "Description": "Collection of VLAN information",
        "Memners": list(),
        "Memners@odata.count": 0,
    },
    '1.4.1.1.1.2.1': {
        "@odata.context": "/redfish/v1/$metadata#VLAN.VLAN",
        "@odata.type": "#VLAN.v1_1_5.VLAN",
        "@odata.id": "",
        "Id": "",
        "Name": "VLAN",
        "Description": "VLAN information",
    },
}
