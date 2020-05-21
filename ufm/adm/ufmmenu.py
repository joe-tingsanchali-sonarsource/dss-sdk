# ufmmenu.py

g_util_name = None

class UfmMenuItem(object):
    def __init__(self, labels=[""], args=[], action=None, desc="", \
        priv=None, hidden=False):
        self.labels = labels
        self.args = args
        self.priv = priv

        if action == None:
            action = self._nop      

        self.action = action
        self.desc = desc
        self.argv = []
        self.hidden = hidden
        return

    def _nop (self, menu, item):
        return

class UfmMenu(object):
    def __init__(self, name=None, title=None, items=[], \
        priv=None, back_func=None, help_func=None, \
        quit_func=None, usage=None):

        self.title = title
        self.name = name
        self.usage = usage

        self.back = back_func
        self.help = help_func
        self.quit = quit_func

        self.priv = priv
        self.cli = False
        self.next_menu = self

        self.argv = []  # cli mode
        self.last_item = UfmMenuItem()
        self.items = []
        self.items += items
        self.default_items = []

        #default items
        item = UfmMenuItem(labels=["q"], action=self._quit_action,\
            desc="Exit the program.")
        self.default_items.append(item)

        item = UfmMenuItem(labels=["h","-h", "-help", "help"], action=self._help_action,\
            desc="Display the help screen.") 
        self.default_items.append(item)

        item = UfmMenuItem(labels=["p"], action=self._prev_action,\
            desc="Repeat previous valid command.")
        self.default_items.append(item)

        item = UfmMenuItem(labels=["b"], action=self._back_action,\
            desc="Return back to parent menu")
        self.default_items.append(item)

        return

    def add_item(self, labels=[""], args=[], action=None, \
            desc="", priv=None, hidden=False):

        self.rem_item(labels[0])

        item = UfmMenuItem(labels=labels, args=args, action=action, \
            desc=desc, priv=priv, hidden=hidden)

        self.items.append(item)
        return

    def rem_item(self, label=""):
        if label != "":
            for item in self.items:
                if label in item.labels:
                    self.items.remove(item)
                    break
        return

    def run(self, argv=[]):
        global g_util_name

        menu = self  # main menu
        cli  = False

        if len(argv) > 0:
            name = argv.pop(0)
            g_util_name = name.split("/")[-1].split(".py")[0]

        menu.argv = argv

        if len(argv) > 0:
            cli = True

        while True:
            if menu.name == None:
                menu.name = g_util_name

            if cli == True:
                menu._cli_mode()

                if len(menu.argv) == 0:
                    menu.set_menu(None)

            else:
                menu._menu_mode()

            if menu.next_menu == None:
                break

            if menu != menu.next_menu:
                menu.next_menu.argv = menu.argv
                menu = menu.next_menu

        return

    def _menu_mode(self):
        while True:
            if self.title != None:
                print()
                print("        "+self.title)

            print()
            print("OPTIONS:")

            # User items
            for item in self.items:
                if item.hidden == True:
                    continue
                if len(item.args) > 0:
                    arg_list = item.labels[0]
                    for arg in item.args:
                        arg_list += " "+arg
                    print("  %-9s :%s" % (arg_list, item.desc))

                elif item.desc != "":
                    print("  %-9s :%s" % (item.labels[0],item.desc))

                else:
                    print()

            if len(self.items) > 0:
                print()

            # Default items
            for item in self.default_items:
                if item.hidden == True:
                    continue
                if len(item.args) > 0:
                    arg_list = item.labels[0]
                    for arg in item.args:
                        arg_list += " "+arg

                    print("  %-9s :%s" % (arg_list,item.desc))

                elif item.desc != "":
                    print("  %-9s :%s" % (item.labels[0],item.desc))

                else:
                    print()

            print()

            while True:
                selection = input(self.name+"> ")
                if selection != "":
                    break

            selection = selection.lower()
            self.argv = selection.split(" ")
            print()

            self._process()

            if self.next_menu != self:
                break

        return  

    def _cli_mode(self):
        self.cli = True
        self._process()
        return  

    def _process(self):
        if len(self.argv) == 0:
            return

        selection = self.argv.pop(0)
        selection = selection.lower()

        found = False
        items = self.items + self.default_items
        for item in items:
            if selection in item.labels:
                found = True
                break
            
        if found == False:
            print()
            print("Invalid command. ("+selection+")")

            if self.cli == True:
                self.set_menu(None)

            return

        item.argv = [selection]

        for arg in item.args:
            arg = arg  #do nothing
            if len(self.argv) == 0:
                print("ERROR: Insufficient number of arguments for this option.")
                return

            selection = self.argv.pop(0)
            selection = selection.lower()
            item.argv.append(selection)

        self.set_menu(self)
        item.action(self, item)

        if item.action != self._prev_action:
            self.last_item = item

        return

    # embedded item action
    def _prev_action(self, menu, item):
        item = self.last_item

        if item.desc == "":
            return

        self.set_menu(self)
        item.action(menu, item)     
        return 

    # embedded item action
    def _help_action(self, menu, item):
        if self.cli == True:
            if (self.usage != None):
                self.usage(menu, item)
            else:
                print()
                print("> No help is available at this time.")

            self.set_menu(None)
            return

        if (self.help != None) and (self.help != self._help_action):
            self.help(menu, item)
        else:
            print()
            print("Choose an option and enter the letter(s) of your choice.")
            print("*NOTE: Normally only the first two characters of an option are needed.")
        return

    # embedded item action
    def _quit_action(self, menu, item):
        if self.quit != None:
            self.quit()
        else:
            print("Exiting")
            print()

        #self.exit = True
        self.set_menu(None)

        return

    # embedded item action
    def _back_action(self, menu, item):
        if self.back != None:
            print("Back func =",self.back)
            self.back(self, menu, item)
            print()

        return

    def set_menu(self, new_menu):
        if new_menu != None:
            new_menu.cli = self.cli
        self.next_menu = new_menu
        return