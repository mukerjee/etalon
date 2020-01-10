"""
A simple CloudLab profile for creating a disk image for use with the
"profile_etalon-ccanel.py" profile.
"""

import geni.portal as portal
import geni.rspec.pg as pg
import geni.rspec.igext as igext

DISK_IMAGE = "urn:publicid:IDN+utah.cloudlab.us+image+emulab-ops//UBUNTU16-64-STD"

pc = portal.Context()
rspec = pg.Request()

pc.defineParameter(
    name="node_type",
    description=("Hardware spec of node. <br> Refer to manuals at "
                 "<a href=\"http://docs.aptlab.net/hardware.html#%28part._apt-cluster%29\">APT</a> "
                 "for more details."),
    typ=portal.ParameterType.NODETYPE,
    defaultValue="r320",
    legalValues=[("r320", "APT r320"), ("c6220", "APT c6220")],
    advanced=False,
    groupId=None)
params = pc.bindParameters()

node = pg.RawPC("host")
node.hardware_type = params.node_type
node.disk_image = DISK_IMAGE
rspec.addResource(node)

instructions = "Use this profile to create a new Etalon disk image with the reTCP kernel patch."
desc = "A very basic profile with a single clean node."
tour = igext.Tour()
tour.Description(type=igext.Tour.TEXT, desc=desc)
tour.Instructions(type=igext.Tour.MARKDOWN, inst=instructions)
rspec.addTour(tour)
pc.printRequestRSpec(rspec)
