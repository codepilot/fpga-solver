#include "InflateGZ.h"
#include "Timer.h"
#include <fstream>
#include "InterchangeFlat.h"

int main(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (auto &&arg: std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	auto dev_gz{ (args.size() >= 2) ? args.at(1) : "_deps/device-file-src/xcvu3p.device" };
	auto dev_flat{ (args.size() >= 3) ? args.at(2) : "_deps/device-file-build/xcvu3p.device" };

{
  const auto dev{ TimerVal(InflateGZ::inflate_mmf( dev_gz, dev_flat )) };
}
	const DevFlat devFlat{ dev_flat };
	const device_reader root{devFlat.root};
	decltype(auto)   name{ root.getName() };           //  @0 : Text;
	decltype(auto)   strs{ root.getStrList() };         //  @1 : List(Text) $hashSet();
	decltype(auto)   siteTypes{ root.getSiteTypeList() };    //  @2 : List(SiteType);
	decltype(auto)   tileTypes{ root.getTileTypeList() };    //  @3 : List(TileType);
	decltype(auto)   tiles{ root.getTileList() };        //  @4 : List(Tile);
	decltype(auto)   wires{ root.getWires() };           //  @5 : List(Wire);
	decltype(auto)   nodes{ root.getNodes() };           //  @6 : List(Node);
	// # Netlist libraries of Unisim primitives and macros
	// # The library containing primitives should be called "primitives", and
	// # the library containing macros called "macros".
	decltype(auto)   primLibs{ root.getPrimLibs() };        //  @7 : Dir.Netlist;
	decltype(auto)   exceptionMap{ root.getExceptionMap() };    //  @8 : List(PrimToMacroExpansion); # Prims to macros expand w/same name, except these
	decltype(auto)   cellBelMap{ root.getCellBelMap() };      //  @9 : List(CellBelMapping);
	decltype(auto)   cellInversions{ root.getCellInversions() };  // @10 : List(CellInversion);
	decltype(auto)   packages{ root.getPackages() };        // @11 : List(Package);
	decltype(auto)   constants{ root.getConstants() };       // @12 : Constants;
	decltype(auto)   constraints{ root.getConstraints() };     // @13 : Constraints;
	decltype(auto)   lutDefinitions{ root.getLutDefinitions() };  // @14 : LutDefinitions;
	decltype(auto)   parameterDefs{ root.getParameterDefs() };   // @15 : ParameterDefinitions;
	decltype(auto)   wireTypes{ root.getWireTypes() };       // @16 : List(WireType);
	decltype(auto)   pipTimings{ root.getPipTimings() };      // @17 : List(PIPTiming);
	decltype(auto)   nodeTimings{ root.getNodeTimings() };     // @18 : List(NodeTiming);

	const size_t   str_count{ strs.size() };         //  @1 : List(Text) $hashSet();
	const size_t   siteType_count{ siteTypes.size() };    //  @2 : List(SiteType);
	const size_t   tileType_count{ tileTypes.size() };    //  @3 : List(TileType);
	const size_t   tile_count{ tiles.size() };        //  @4 : List(Tile);
	const size_t   wire_count{ wires.size() };           //  @5 : List(Wire);
	const size_t   node_count{ nodes.size() };           //  @6 : List(Node);
	const size_t   exceptionMap_count{ exceptionMap.size() };    //  @8 : List(PrimToMacroExpansion); # Prims to macros expand w/same name, except these
	const size_t   cellBelMap_count{ cellBelMap.size() };      //  @9 : List(CellBelMapping);
	const size_t   cellInversion_count{ cellInversions.size() };  // @10 : List(CellInversion);
	const size_t   package_count{ packages.size() };        // @11 : List(Package);
	const size_t   wireType_count{ wireTypes.size() };       // @16 : List(WireType);
	const size_t   pipTiming_count{ pipTimings.size() };      // @17 : List(PIPTiming);
	const size_t   nodeTiming_count{ nodeTimings.size() };     // @18 : List(NodeTiming);


	std::ofstream header("lib_dev_flat.h", std::ios::binary);
	header << std::format(R"(#pragma once
#include "InterchangeFlat.h"

namespace {} {{
inline static constexpr decltype(auto)   gz_path{{ "{}" }};
inline static constexpr decltype(auto)   flat_path{{ "{}" }};
inline static const     DevFlat          flat{{ flat_path }};
inline static const     device_reader    root{{flat.root}};

inline static constexpr decltype(auto) name{{ "{}" }}; //  @0 : Text;
inline static decltype(auto)   strs{{ root.getStrList() }};            //  @1 : List(Text) $hashSet();
inline static decltype(auto)   siteTypes{{ root.getSiteTypeList() }};       //  @2 : List(SiteType);
inline static decltype(auto)   tileTypes{{ root.getTileTypeList() }};       //  @3 : List(TileType);
inline static decltype(auto)   tiles{{ root.getTileList() }};           //  @4 : List(Tile);
inline static decltype(auto)   wires{{ root.getWires() }};           //  @5 : List(Wire);
inline static decltype(auto)   nodes{{ root.getNodes() }};           //  @6 : List(Node);
  // # Netlist libraries of Unisim primitives and macros
  // # The library containing primitives should be called "primitives", and
  // # the library containing macros called "macros".
inline static decltype(auto)   primLibs{{ root.getPrimLibs() }};        //  @7 : Dir.Netlist;
inline static decltype(auto)   exceptionMap{{ root.getExceptionMap() }};    //  @8 : List(PrimToMacroExpansion); # Prims to macros expand w/same name, except these
inline static decltype(auto)   cellBelMap{{ root.getCellBelMap() }};      //  @9 : List(CellBelMapping);
inline static decltype(auto)   cellInversions{{ root.getCellInversions() }};  // @10 : List(CellInversion);
inline static decltype(auto)   packages{{ root.getPackages() }};        // @11 : List(Package);
inline static decltype(auto)   constants{{ root.getConstants() }};       // @12 : Constants;
inline static decltype(auto)   constraints{{ root.getConstraints() }};     // @13 : Constraints;
inline static decltype(auto)   lutDefinitions{{ root.getLutDefinitions() }};  // @14 : LutDefinitions;
inline static decltype(auto)   parameterDefs{{ root.getParameterDefs() }};   // @15 : ParameterDefinitions;
inline static decltype(auto)   wireTypes{{ root.getWireTypes() }};       // @16 : List(WireType);
inline static decltype(auto)   pipTimings{{ root.getPipTimings() }};      // @17 : List(PIPTiming);
inline static decltype(auto)   nodeTimings{{ root.getNodeTimings() }};     // @18 : List(NodeTiming);


inline static constexpr size_t str_count {{ {}ull }};           //  @1 : List(Text) $hashSet();
inline static constexpr size_t siteType_count{{ {}ull }};       //  @2 : List(SiteType);
inline static constexpr size_t tileType_count{{ {}ull }};       //  @3 : List(TileType);
inline static constexpr size_t tile_count{{ {}ull }};           //  @4 : List(Tile);
inline static constexpr size_t wire_count{{ {}ull }};           //  @5 : List(Wire);
inline static constexpr size_t node_count{{ {}ull }};           //  @6 : List(Node);
inline static constexpr size_t exceptionMap_count{{ {}ull }};   //  @8 : List(PrimToMacroExpansion); # Prims to macros expand w/same name, except these
inline static constexpr size_t cellBelMap_count{{ {}ull }};     //  @9 : List(CellBelMapping);
inline static constexpr size_t cellInversion_count{{ {}ull }};  // @10 : List(CellInversion);
inline static constexpr size_t package_count{{ {}ull }};        // @11 : List(Package);
inline static constexpr size_t wireType_count{{ {}ull }};       // @16 : List(WireType);
inline static constexpr size_t pipTiming_count{{ {}ull }};      // @17 : List(PIPTiming);
inline static constexpr size_t nodeTiming_count{{ {}ull }};     // @18 : List(NodeTiming);


}};

)",
name.cStr(),
dev_gz,
dev_flat,
name.cStr(),
str_count,
siteType_count,
tileType_count,
tile_count,
wire_count,
node_count,
exceptionMap_count,
cellBelMap_count,
cellInversion_count,
package_count,
wireType_count,
pipTiming_count,
nodeTiming_count
);

	std::ofstream cpp("lib_dev_flat.cpp", std::ios::binary);
	cpp << std::format(R"(
#define BUILDING_lib_dev_flat
#include "lib_dev_flat.h"

)");

	return 0;
}