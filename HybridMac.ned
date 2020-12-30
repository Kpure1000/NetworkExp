//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

network HybridMac{
    parameters:

        double width;
        double height;      //  net work size

        int dataNodeNum;    //  Node numbers
        int queueSize;      //  size of queue
        double rounds;      //  TODO: ?

        double CW_checkInterval;    //  interval imte of collision window
        int CWmin;                  //  total interval time of collision window
        
        double DIFS_checkInterval;  //  DIFS interval time
        double DIFS;                //  DIFS time

        double data_packet_transmission_time;       //  transmission time of data
        double conflict_time;                       //  conflict time
        int retryLimit;                             //  retry limit times
        int backoffStageLimit;                      //  binary backoff indices limit
        double lambda;                              //  create packet; Poison
        double packetSize;                          //  size of packet
        double networkSpeed;                        //  spreed speed of network

    submodules:
        node[dataNodeNum] : Node{
        }

    connections allowunconnected:
}