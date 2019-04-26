
import geni.portal as portal
import geni.rspec.pg as pg
import geni.rspec.igext as igext

SWITCH_DISK_IMAGE = "urn:publicid:IDN+utah.cloudlab.us+image+emulab-ops//UBUNTU16-64-STD"
NODE_DISK_IMAGE = "urn:publicid:IDN+apt.emulab.net+image+dna-PG0:Etalon"

pc = portal.Context()
rspec = pg.Request()

pc.defineParameter(
    "node_type",
    ("Hardware spec of nodes <br> Refer to manuals at "
     "<a href=\"http://docs.aptlab.net/hardware.html#%28part._apt-cluster%29\">APT</a> for more "
     "details."),
    portal.ParameterType.NODETYPE,
    "r320",
    legalValues=[("r320", "APT r320"), ("c6220", "APT c6220")],
    advanced=False,
    groupId=None)
pc.defineParameter(
    "num_nodes",
    ("Number of hosts (not including OCS) to use.<br> Check cluster availability "
     "<a href=\"https://www.cloudlab.us/cluster-graphs.php\">here</a>."),
    portal.ParameterType.INTEGER,
    8,
    legalValues=[],
    advanced=False,
    groupId=None)
pc.defineParameter(
    "switch",
    "Preferred Mellanox switch",
    portal.ParameterType.STRING,
    "mellanox3",
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

nodes = []
lan = pg.LAN("lan-1")

nodes.append(pg.RawPC("switch"))
iface = nodes[-1].addInterface("if-switch", pg.IPv4Address("10.2.100.100", "255.255.255.0"))
nodes[-1].hardware_type = params.node_type
nodes[-1].disk_image = SWITCH_DISK_IMAGE
nodes[-1].Desire(params.switch, 1.0)
nodes[-1].addService(
    pg.Install("https://github.com/mukerjee/etalon/archive/master.tar.gz", "/local/"))
# nodes[-1].addService(pg.Execute("/bin/bash","/local/etalon-master/cloudlab/switch_install.sh"))
lan.addInterface(iface)
rspec.addResource(nodes[-1])

for i in range(1, params.num_nodes + 1):
    nodes.append(pg.RawPC("host%s" % i))
    iface = nodes[i].addInterface("if-host%s" % i,
                                  pg.IPv4Address("10.2.100.%d" % i,
                                                 "255.255.255.0"))
    nodes[i].hardware_type = params.node_type
    nodes[i].disk_image = NODE_DISK_IMAGE
    nodes[i].Desire(params.switch, 1.0)
    nodes[i].addService(
        pg.Install("https://github.com/mukerjee/etalon/archive/master.tar.gz", "/local/"))
    # nodes[i].addService(pg.Execute("/bin/bash","/local/etalon-master/cloudlab/node_install.sh"))
    lan.addInterface(iface)
    rspec.addResource(nodes[i])

rspec.addResource(lan)

instructions = ("Use this profile to instantiate a set of nodes on the APT cluster, all bound to "
                "the same mellanox switch, for use with Etalon.")
desc = "Etalon Reconfigurable Datacenter Emulator."
tour = igext.Tour()
tour.Description(type=igext.Tour.TEXT, desc=desc)
tour.Instructions(type=igext.Tour.MARKDOWN, inst=instructions)
rspec.addTour(tour)
pc.printRequestRSpec(rspec)
