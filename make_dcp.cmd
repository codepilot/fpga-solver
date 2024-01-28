call rapidwright.bat PhysicalNetlistToDcp "build\_deps\benchmark-files-src\boom_med_pb.netlist" "build\dst_written.phy.gz" "empty.xdc" out2.dcp
call vivado.bat -mode batch -source report_route_status.tcl out2.dcp
