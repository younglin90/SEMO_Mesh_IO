
#include "mesh_io.hpp"




int main() {

	semo::mesh msh;
	semo::mesh_io io;


	//msh << io.load("./mesh/openfoam");
	msh << io.load("./mesh/300_polygon_sphere_100mm.STL");
	unique_vertex(msh);

	msh >> io.save("./mesh/Bunny_copy.obj");

	std::cout << msh.pos.size() << std::endl;
	std::cout << msh.f2v.size() << std::endl;

	//msh << io.load("asdfadf.Obj");

	//msh << io.load("asdfadf.Obj");
	//msh >> io.save("asdfadf.stl");


	return 0;
}