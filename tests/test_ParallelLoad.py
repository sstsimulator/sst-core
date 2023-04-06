import sst
import sys

# Define SST core options
sst.setProgramOption("stop-at", "10us")
sst.setProgramOption("partitioner", "self")

# Size per rank.  They will be tiled linearly in the x dimension
x_size = 4
y_size = 2
#x_size = int(sys.argv[1])
#y_size = int(sys.argv[2])

# Get the number of ranks
num_ranks = sst.getMPIRankCount()
num_threads = sst.getThreadCount()
my_rank = sst.getMyMPIRank()

total_x_size = x_size * num_ranks * num_threads

options = sst.getProgramOptions()
parallel_load = options["parallel-load"]

        
# Set up a map of links with accessor
links = dict()
def getLink(leftName, rightName):
    name = "link_%s_%s"%(leftName, rightName)
    #print(my_rank, name)
    if name not in links:
        links[name] = sst.Link(name)
    return links[name]
        
current_x = -1
# Track whether or not to hook up pos and neg x links.  Once set to
# True, it will stay True
for rank in range(num_ranks):
    x_pos_rank = (rank + 1) % num_ranks
    x_neg_rank = (rank + num_ranks - 1) % num_ranks
    
    # Track whether x connections off rank are possible.
    possible_x_pos_r = x_pos_rank == my_rank
    possible_x_neg_r = x_neg_rank == my_rank
    
    for thread in range(num_threads):
        possible_x_pos_t = possible_x_pos_r and thread == (num_threads - 1)
        possible_x_neg_t = possible_x_neg_r and thread == 0

        for x in range (x_size):
            current_x += 1

            # After this, these are no longer possible, but actual
            x_pos_link = possible_x_pos_t and x == (x_size - 1)
            x_neg_link = possible_x_neg_t and x == 0

            # If we're loading in parallel, we only continue if this
            # is my_rank or one of the possible_x_* is True
            if parallel_load and not (rank == my_rank or x_pos_link or x_neg_link):
                continue
            
            for y in range(y_size):

                # If we made it here, the component needs to be instanced
                #print(my_rank, "comp_x%d_y%d"%(current_x,y))
                comp = sst.Component("comp_x%d_y%d"%(current_x,y), "coreTestElement.message_mesh.enclosing_component")
                comp.addParam("id", y * total_x_size + current_x)
                comp.setRank(rank,thread)

                if not parallel_load or my_rank == rank:
                    # Setup the route subcomponent
                    route = comp.setSubComponent("route","coreTestElement.message_mesh.route_message")
                
                # Whether we connect X links or not will depend on our
                # position and if parallel loading is on or not.  If
                # parallel_loading is on, then we will only instance
                # links that touch a component in the current rank

                # X pos
                if ( my_rank == rank or x_pos_link or not parallel_load ):
                    port_x_pos = comp.setSubComponent("ports","coreTestElement.message_mesh.message_port",0);
                    their_x = current_x + 1
                    if their_x == total_x_size:
                        their_x = 0
                    port_x_pos.addLink(getLink("x%dy%d"%(current_x, y), "x%dy%d"%(their_x, y)), "port", "1ns")
                     
                # X neg
                if ( my_rank == rank or x_neg_link or not parallel_load ):
                    port_x_neg = comp.setSubComponent("ports","coreTestElement.message_mesh.message_port",1);
                    their_x = current_x - 1
                    if their_x == -1:
                        their_x = total_x_size - 1
                    port_x_neg.addLink(getLink("x%dy%d"%(their_x, y), "x%dy%d"%(current_x, y)), "port", "1ns")

                # Y links
                # only hook up the y link for parallel loading when this is our rank
                if ( parallel_load and rank != my_rank ):
                    continue;

                # Positive
                port_y_pos = comp.setSubComponent("ports","coreTestElement.message_mesh.message_port",2);
    
                their_y = y + 1
                if their_y == y_size:
                    their_y = 0

                port_y_pos.addLink(getLink("x%dy%d"%(current_x, y), "x%dy%d"%(current_x,their_y)), "port", "1ns")

                # Negative
                port_y_neg = comp.setSubComponent("ports","coreTestElement.message_mesh.message_port",3);

                their_y = y - 1
                if their_y == -1:
                    their_y = y_size - 1
                port_y_neg.addLink(getLink("x%dy%d"%(current_x,their_y), "x%dy%d"%(current_x, y)), "port", "1ns")
