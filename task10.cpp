#include <boost/asio.hpp>
#include <iostream>
#include <fstream>

using boost::asio::ip::tcp;

class Node {
public:
	Node(int nodeNumber) {
		this->nodeNumber = nodeNumber;
	}

	// Initialize the node withe the given routing table
	// Note that the algorithm assumes an identical routing table on all nodes.
	void setup(char const *routingTableName) {
		// Setup the networking system
		ioService = new boost::asio::io_service();
		readRoutingTable(routingTableName);
		try {
			acceptor = new tcp::acceptor(*ioService, routingTable[nodeNumber]);
		} catch (std::exception &) {
			std::cout << "error listening on " << routingTable[nodeNumber] << std::endl;
			throw;
		}
	}

	void run() {
        std::cout << "NodeNumber: " << nodeNumber << std::endl;
        int amountOfNodes = routingTable.size();
        int sum = 0;
     
        //------------------------------------
        //calculate receives
        int tester = 1;       
        int receives = 0;
        while((nodeNumber & tester) == 0 && (nodeNumber + tester) < amountOfNodes){
            ++receives;
            tester = tester << 1;
        }
        while((nodeNumber & tester) == 0){
            tester = tester << 1;
        }
        
        std::cout << "receives: " << receives << std::endl;
        //------------------------------------

        if(nodeNumber == 0) {            
            int input = 0;
            printf("x[0]: ");
            std::cin >> sum;

            for(int i(1); i < amountOfNodes; ++i){
                std::cout << "x[" << i << "]: ";
                std::cin.clear();
                std::cin >> input;
                sendDataToNode(input, i);
            }

        } else {  
            std::cout << "send to node: " << (nodeNumber - tester) << std::endl;
            sum = receiveData();     
            std::cout << "received: " << sum << std::endl;
        }
        
        while(receives){
            int r = receiveData();
            std::cout << "r: " << r << std::endl;
            sum += r;            
            std::cout << --receives << "receives left " << std::endl;
        }
        
        if(nodeNumber == 0){
            std::cout << "sum: " << sum << std::endl;
        }
        else
        {
            sendDataToNode(sum, nodeNumber - tester);             
        }

        std::cin.clear();
        int e;
        std::cin >> e;                
	}

private:
	// Sends the given data to the given target node and waits
	// until the data is received by the remote endpoint.
	void sendDataToNode(int x, int targetNodeNumber) {
		std::stringstream dataStream;
		dataStream << x;
		try {
			tcp::socket socket(*ioService);
			socket.connect(routingTable[targetNodeNumber]);
			socket.send(boost::asio::buffer(dataStream.str()));
			socket.shutdown(tcp::socket::shutdown_both);
			socket.close();
		} catch (std::exception &) {
			std::cout << "error sending data to " << targetNodeNumber << ": " << routingTable[targetNodeNumber] << std::endl;
			throw;
		}
	}

	// Waits until data from any incoming node is received and
	// returns the received data.
	// Note that this functions accepts data from any endpoint even if
	// this endpoint is not entered in the routing table.
	int receiveData() {
		int x;
		char buffer[32];
		tcp::socket socket(*ioService);
		try {
			acceptor->accept(socket);
			socket.receive(boost::asio::buffer(buffer, 32));
			socket.shutdown(tcp::socket::shutdown_both);
			socket.close();
			std::stringstream dataStream(buffer);
			dataStream >> x;
			return x;
		} catch (std::exception &) {
			std::cout << "error receiving data." << std::endl;
			throw;
		}
	}

	// Reads the routing table from the given file.
	// The number of nodes envolved in the calculation is derived from
	// the routing table.
	void readRoutingTable(char const *routingTableName) {
		char line[1024];
		std::ifstream file;
		std::string ipAddress, port;
		int nodeNumber = 0;
		tcp::resolver resolver(*ioService);

		file.open(routingTableName);
		while (!file.eof()) {
			file.getline(line, 1024);
			std::stringstream lineStream(line);
			// ignore empty lines
			if (lineStream.str().length() == 0) break;
			// ignore lines that start with '#'
			if (lineStream.str().at(0) == '#') break;
			lineStream >> ipAddress;
			lineStream >> port;

			std::cout << nodeNumber << "= " << ipAddress << ":" << port << std::endl;

			tcp::resolver::query query(tcp::v4(), ipAddress, port);
			try {
				routingTable.push_back(*resolver.resolve(query));
			} catch (std::exception &cause) {
				std::cout << nodeNumber << ": " << cause.what() << std::endl;
			}
			++nodeNumber;
		}
	}

	// Node topology data
	int nodeNumber;

	// network data
	boost::asio::io_service *ioService;
	std::vector<tcp::endpoint> routingTable;
	tcp::acceptor *acceptor;
};


int main(int argc, char const *argv[]) {
	int nodeNumber = 0;
	char const *routingTableName = "routingTable.txt";
	if (argc > 1) {
		std::stringstream argumentStream(argv[1]);
		argumentStream >> nodeNumber;
	}
	if (argc > 2) {
		routingTableName = argv[2];
	}

	try {
		Node node(nodeNumber);
		node.setup(routingTableName);
		node.run();
	} catch (std::exception &cause) {
		std::cout << cause.what() << std::endl;
	}

	return 0;
}
