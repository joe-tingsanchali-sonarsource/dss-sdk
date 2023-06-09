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


from ufmmenu import UfmMenu
import ufmapi


class VlansMenu(UfmMenu):
    def __init__(self, fab="", sw=""):
        UfmMenu.__init__(self, name="vlans", back_func=self._back_action)

        self.fab = fab
        self.sw = sw
        self._refresh()
        return

    def _refresh(self):
        rsp = ufmapi.redfish_get(
            "/Fabrics/" + self.fab + "/Switches/" + self.sw + "/VLANs")

        if rsp is None:
            return

        if "Members" not in rsp:
            return

        count = 0
        print()
        print("*  VLANs Collection:")

        for member in rsp["Members"]:
            vlan = member["@odata.id"].split("/")[8]
            vlan_display = "VLAN " + vlan
            print("      VLAN: (" + str(count) + ")", vlan_display)

            self.add_item(labels=[str(count)],
                          action=self._menu_action, priv=vlan, desc=vlan_display)

            count = count + 1

        if "Actions" in rsp and rsp["Actions"]["#CreateVLAN"]:

            crt = rsp["Actions"]["#CreateVLAN"]
            print("    Action: (CreateVLAN) ")
            self.add_item(labels=["crt", "crt"], args=["<vlan_id>"],
                          action=self._create_action,
                          desc=crt["description"])
            self.crt = crt

    def _back_action(self, menu, item):
        from ufmadm_switches import SwitchMenu

        switch_menu = SwitchMenu(fab=self.fab, sw=self.sw)
        self.set_menu(switch_menu)

    def _menu_action(self, menu, item):
        vlan_menu = VlanMenu(fab=self.fab, sw=self.sw, vlan=item.priv)
        self.set_menu(vlan_menu)

    def _create_action(self, menu, item):
        argv = item.argv

        vlan_id = int(argv[1], 10)
        if vlan_id is None:
            print("VLAN ID Undefined, invalid or missing")
            return

        payload = {}
        payload["VLANId"] = vlan_id

        rsp = ufmapi.redfish_post(self.crt["target"], payload)

        succeeded = ufmapi.print_switch_result(rsp,
                                               'vlan ' + str(vlan_id),
                                               'VLAN created',
                                               'Failed to create VLAN')
        if succeeded:
            self._refresh()


class VlanMenu(UfmMenu):
    def __init__(self, fab="", sw="", vlan=""):
        if vlan is None:
            print("ERROR: Null vlan provided.")
            return

        if sw is None:
            print("ERROR: Null Switch provided.")
            return

        if fab is None:
            print("ERROR: Null Fabric provided.")
            return

        UfmMenu.__init__(self, name="vlan", back_func=self._back_action)

        self.fab = fab
        self.sw = sw
        self.vlan = vlan

        self._refresh()

    def _refresh(self):
        rsp = ufmapi.redfish_get("/Fabrics/" + self.fab + "/Switches/" + self.sw + "/VLANs/" + self.vlan)

        if rsp is None:
            return

        if "Id" not in rsp:
            return

        print()
        print("*        Fabric: ", self.fab)
        print("*        Switch: ", self.sw)
        print("             Id: ", rsp["Id"])
        print("    Description: ", rsp["Description"])

        if rsp["Name"]:
            print("           Name: ", rsp["Name"])
        else:
            print("           Name: (empty)")

        print("     VLANEnable: ", rsp["VLANEnable"])
        print("         VLANId: ", rsp["VLANId"])
        print()

        if "Actions" in rsp and rsp["Actions"]["#DeleteVLAN"]:

            dlt = rsp["Actions"]["#DeleteVLAN"]
            print("    Action: (DeleteVLAN) ")
            self.add_item(labels=["dlt", "dlt"],
                          action=self._delete_vlan_action,
                          desc=dlt["description"])
            self.dlt = dlt

        if "Actions" in rsp and rsp["Actions"]["#NameVLAN"]:

            nam = rsp["Actions"]["#NameVLAN"]
            print("    Action: (NameVLAN) ")
            self.add_item(labels=["nam", "nam"], args=["<vlan_name>"],
                          action=self._name_vlan_action,
                          desc=nam["description"])
            self.nam = nam

        if "Links" in rsp and rsp["Links"]["Ports"]:
            if len(rsp["Links"]["Ports"]) > 0:
                count = 0
                print()
                print("*  Ports Collection:")

                for member in rsp["Links"]["Ports"]:
                    pt = member["@odata.id"].split("/")[8]
                    pt_display = 'Eth1/' + pt
                    print("      Port: (" + str(count) + ")", pt_display)

                    self.add_item(labels=[str(count)],
                                  action=self._menu_action, priv=pt,
                                  desc="Port: " + pt_display)

                    count = count + 1

    def _back_action(self, menu, item):
        switches_menu = VlansMenu(fab=self.fab, sw=self.sw)
        self.set_menu(switches_menu)

    def _menu_action(self, menu, item):
        from ufmadm_ports import PortMenu

        port_menu = PortMenu(fab=self.fab, sw=self.sw, pt=item.priv)
        self.set_menu(port_menu)

    def _delete_vlan_action(self, menu, item):
        rsp = ufmapi.redfish_post(self.dlt["target"])

        succeeded = ufmapi.print_switch_result(rsp,
                                               'no vlan ' + str(self.vlan),
                                               'VLAN deleted',
                                               'Failed to delete VLAN')
        if succeeded:
            self._back_action(menu, item)

    def _name_vlan_action(self, menu, item):
        argv = item.argv

        vlan_name = argv[1]
        if vlan_name is None:
            print("VLAN name Undefined, invalid or missing")
            return

        payload = {}
        payload["Name"] = vlan_name
        payload["VLANId"] = self.vlan

        rsp = ufmapi.redfish_post(self.nam["target"], payload)

        succeeded = ufmapi.print_switch_result(rsp,
                                               'vlan ' + str(self.vlan) + ' name ' + vlan_name,
                                               'Successfully named VLAN',
                                               'Failed to name VLAN')
        if succeeded:
            self._refresh()
