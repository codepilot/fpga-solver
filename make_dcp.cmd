@REM rapidwright PhysicalNetlistToDcp "C:\Users\root\Desktop\fpga-solver\build\_deps\benchmark-files-src\boom_soc.netlist" "C:\Users\root\Desktop\fpga-solver\build\_deps\benchmark-files-src\boom_soc_unrouted.phys" "C:\Users\root\Desktop\fpga-solver\empty.xdc" out.dcp
@REM call rapidwright PhysicalNetlistToDcp "C:\Users\root\Desktop\fpga-solver\build\_deps\benchmark-files-src\boom_soc.netlist" "C:\Users\root\Desktop\fpga-solver\build\dst_written.phy.gz" "C:\Users\root\Desktop\fpga-solver\empty.xdc" out2.dcp
call rapidwright PhysicalNetlistToDcp "C:\Users\root\Desktop\fpga-solver\build\_deps\benchmark-files-src\boom_med_pb.netlist" "C:\Users\root\Desktop\fpga-solver\build\dst_written.phy.gz" "C:\Users\root\Desktop\fpga-solver\empty.xdc" out2.dcp
call D:\Xilinx\Vivado\2023.2\bin\vivado.bat -mode batch -source report_route_status.tcl out2.dcp
