import sst
import sys

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")

x_size = int(sys.argv[1])
y_size = int(sys.argv[2])

# Calculate number of routers and endpoints
num_routers = x_size * y_size
        
# Set up a map of links with accessor
links = dict()
def getLink(leftName, rightName):
    name = "link_%s_%s"%(leftName, rightName)
    if name not in links:
        links[name] = sst.Link(name)
    return links[name]
        
for i in range(num_routers):
    my_x = i % x_size
    my_y = i // x_size


    comp = sst.Component("component%d"%i, "coreTestElement.message_mesh.enclosing_component")
    comp.addParam("id",i)
    
    # Setup up all the ports.  X ports will use MessagePort directly, Y ports, will use the SlotPort
    port_x_pos = comp.setSubComponent("ports","coreTestElement.message_mesh.message_port",0);
    port_x_neg = comp.setSubComponent("ports","coreTestElement.message_mesh.message_port",1);

    tmp = comp.setSubComponent("ports","coreTestElement.message_mesh.port_slot",2);
    port_y_pos = tmp.setSubComponent("port","coreTestElement.message_mesh.message_port");
    
    tmp = comp.setSubComponent("ports","coreTestElement.message_mesh.port_slot",3);
    port_y_neg = tmp.setSubComponent("port","coreTestElement.message_mesh.message_port");
    
    # Setup the route subcomponent
    route = comp.setSubComponent("route","coreTestElement.message_mesh.route_message")


    # Put in the links

    # X-dim
    # Positive
    their_x = my_x + 1
    if their_x == x_size:
        their_x = 0
    port_x_pos.addLink(getLink("x%dy%d"%(my_x,my_y), "x%dy%d"%(their_x,my_y)), "port", "1ns")
    # Set the nocut attribute on positive x-link on every other router
    if ( i % 2 == 0):
        getLink("x%dy%d"%(my_x,my_y), "x%dy%d"%(their_x,my_y)).setNoCut()
    
    # Negative
    their_x = my_x - 1
    if their_x == -1:
        their_x = x_size - 1
    port_x_neg.addLink(getLink("x%dy%d"%(their_x,my_y), "x%dy%d"%(my_x,my_y)), "port", "1ns")


    # Y-dim
    # Positive
    their_y = my_y + 1
    if their_y == y_size:
        their_y = 0
    port_y_pos.addLink(getLink("x%dy%d"%(my_x,my_y), "x%dy%d"%(my_x,their_y)), "port", "1ns")

    # Negative
    their_y = my_y - 1
    if their_y == -1:
        their_y = y_size - 1
    port_y_neg.addLink(getLink("x%dy%d"%(my_x,their_y), "x%dy%d"%(my_x,my_y)), "port", "1ns")
