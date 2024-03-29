
#include "mesh_io.hpp"




int main() {

	semo::mesh msh;
	semo::mesh_io io;


	msh << io.load("./mesh/openfoam");

	msh >> io.save("./mesh/Bunny_copy.vtu");

	std::cout << msh.pos.size() << std::endl;
	std::cout << msh.f2v.size() << std::endl;

	//msh << io.load("asdfadf.Obj");

	//msh << io.load("asdfadf.Obj");
	//msh >> io.save("asdfadf.stl");


	return 0;
}