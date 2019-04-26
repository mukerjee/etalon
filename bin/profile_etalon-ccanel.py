""" A CloudLab profile that creates an Etalon cluster. """

import geni.portal as portal
import geni.rspec.pg as pg
import geni.rspec.igext as igext

SWITCH_DISK_IMAGE = "urn:publicid:IDN+utah.cloudlab.us+image+emulab-ops//UBUNTU16-64-STD"
NODE_DISK_IMAGE = "urn:publicid:IDN+apt.cloudlab.us+image+dna-PG0:etalon"

pc = portal.Context()
rspec = pg.Request()

pc.defineParameter(
    name="node_type",
    description=("Hardware spec of nodes.<br> Refer to manuals at "
                 "<a href=\"http://docs.aptlab.net/hardware.html#%28part._apt-cluster%29\">APT</a> "
                 "for more details."),
    typ=portal.ParameterType.NODETYPE,
    defaultValue="r320",
    legalValues=[("r320", "APT r320"), ("c6220", "APT c6220")],
    advanced=False,
    groupId=None)
pc.defineParameter(
    name="num_nodes",
    description=("Number of hosts (not including OCS) to use.<br> Check cluster availability "
                 "<a href=\"https://www.cloudlab.us/cluster-graphs.php\">here</a>."),
    typ=portal.ParameterType.INTEGER,
    defaultValue=8,
    legalValues=[],
    advanced=False,
    groupId=None)
pc.defineParameter(
    name="switch",
    description="Preferred Mellanox switch",
    typ=portal.ParameterType.STRING,
    defaultValue="mellanox3",
    legalValues=[("mellanox1", "Switch 1"),
                 ("mellanox2", "Switch 2"), ("mellanox3", "Switch 3"),
                 ("mellanox4", "Switch 4"), ("mellanox5", "Switch 5"),
                 ("mellanox6", "Switch 6"), ("mellanox7", "Switch 7"),
                 ("mellanox8", "Switch 8"), ("mellanox9", "Switch 9")],
    advanced=False,
    groupId=None)

params = pc.bindParameters()
if params.num_nodes < 1:
    pc.reportError(portal.ParameterError("You must choose a minimum of 1 node "))
pc.verifyParameters()

lan = pg.LAN("lan")
nodes = []

switch_node = pg.RawPC("switch")
iface = switch_node.addInterface("if-switch", pg.IPv4Address("10.2.100.100", "255.255.255.0"))
lan.addInterface(iface)

switch_node.hardware_type = params.node_type
switch_node.disk_image = SWITCH_DISK_IMAGE
switch_node.Desire(params.switch, 1.0)
switch_node.addService(
    pg.Install("https://github.com/ccanel/etalon/archive/master.tar.gz", "/local/"))
switch_node.addService(pg.Execute("/bin/bash", "/local/etalon-master/bin/switch_install.sh"))
nodes.append(switch_node)
rspec.addResource(switch_node)

for i in range(1, params.num_nodes + 1):
    node = pg.RawPC("host%s" % i)
    iface = node.addInterface("if-host%s" % i, pg.IPv4Address("10.2.100.%d" % i, "255.255.255.0"))
    lan.addInterface(iface)

    node.hardware_type = params.node_type
    node.disk_image = NODE_DISK_IMAGE
    node.Desire(params.switch, 1.0)
    node.addService(
        pg.Install("https://github.com/ccanel/etalon/archive/master.tar.gz", "/local/"))
    node.addService(pg.Execute("/bin/bash", "/local/etalon-master/bin/node_install.sh"))
    nodes.append(node)
    rspec.addResource(node)
rspec.addResource(lan)

instructions = ("Use this profile to instantiate a set of nodes on the APT cluster, all bound to "
                "the same mellanox switch, for use with Etalon.")
desc = "Etalon Reconfigurable Datacenter Emulator."
tour = igext.Tour()
tour.Description(type=igext.Tour.TEXT, desc=desc)
tour.Instructions(type=igext.Tour.MARKDOWN, inst=instructions)
rspec.addTour(tour)
pc.printRequestRSpec(rspec)
