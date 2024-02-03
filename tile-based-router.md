# Tile-Based-Router

* Each thread owns 1 tile  
* Only tiles that have pips are assigned threads  
* Each tile/thread has an **atomic** inbox

## Tile Inbox ring buffer

* atomic ushort2 inbox_read_pos, inbox_write_pos
* ushort[tile wire count] tile wires[index] of new net (inbox_length)

## Tile Wires  

* uint net_id (protected by atomic compare exchange)
* ushort3 previous_tile col, row, wire_idx

## Backtrack/bannded net_ids

* ushort banned_net_offset
* uint2[**chosen_constant?**] list of banned net_ids/wire_idx due to backtracking  

## Nets Buffer

`net = nets[net_id]`  

* ushort2 source tile col, row  

## Tile compute shader high-level activity

* iterate through the inbox(wire_idx, net_id)
  * find a next_hop for the net_id
    * if found  
      * add to next tile
    * if not found, ban the net_id/wire_idx  
      * go to previous tile, add the wire_idx to it's inbox  
