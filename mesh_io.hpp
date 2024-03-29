#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cstdlib>
#include <utility>
#include <functional>
#include <algorithm>
#include <map>
#include <array>
#include <filesystem>
#include <fstream>
#include <tuple>
#include <optional>

namespace semo {


	class mesh;
	void make_c2f_from_f2c(mesh& msh);
	void make_c2v_from_c2f_f2v(mesh& msh);


	// trim from left 
	std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v")
	{
		s.erase(0, s.find_first_not_of(t));
		return s;
	}
	// trim from right 
	std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
	{
		s.erase(s.find_last_not_of(t) + 1);
		return s;
	}
	// trim from left & right 
	std::string& trim(std::string& s, const char* t = " \t\n\r\f\v")
	{
		return ltrim(rtrim(s, t), t);
	}


	std::string get_file_extension(const std::string& filename) {
		auto pos = filename.rfind('.');
		if (pos == 0){//std::string::npos) {
			return "";
		}
		return filename.substr(pos + 1);
	}

	void to_lower(std::string& str) {
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	}


	using pos_t = std::vector<std::array<double, 3>>;
	using f2v_t = std::vector<std::vector<size_t>>;
	using f2c_t = std::vector<std::vector<size_t>>;
	using c2v_t = std::vector<std::vector<size_t>>;
	using c2f_t = std::vector<std::vector<size_t>>;

	using load_t = std::function<void(mesh& m)>; //std::tuple<pos_t, f2v_t>;
	using save_t = std::function<void(mesh& m)>;


	class mesh {
	public:
		pos_t pos;
		f2v_t f2v;
		f2c_t f2c;
		c2v_t c2v;
		c2f_t c2f;

		mesh& operator<<(const std::optional<load_t>& mesh_io) { // 함수 정의
			if (!mesh_io.has_value()) {
				std::cout << "mesh_io is nullptr" << std::endl;
				return *this;
			}
			mesh_io.value()(*this);
			//auto& [pos_io, f2v_io] = mesh_io.value();
			//this->pos = std::move(pos_io);
			//this->f2v = std::move(f2v_io);

			return *this;
		}


		mesh& operator>>(const std::optional<save_t>& mesh_io) { // 함수 정의
			if (!mesh_io.has_value()) {
				std::cout << "mesh_io is nullptr" << std::endl;
				return *this;
			}
			mesh_io.value()(*this);

			return *this;
		}

	};


	class mesh_io_base {
	public:
		virtual load_t load(std::string fileName) = 0;
		virtual save_t save(std::string filename) = 0;
	};

	class stl_mesh_io : public mesh_io_base {
	public:
		load_t load(std::string fileName) override {


			load_t load_func;

			load_func = [fileName](mesh& msh) {


				// 파일 경로 지정
				std::filesystem::path file_path(fileName);

				std::ifstream input_file(file_path, std::ios::in);
				if (!input_file) {
					std::cerr << "Error: Cannot open file " << file_path << '\n';
					return;
				}

				char buf[6]{};
				buf[5] = 0;
				input_file.read(buf, 5);
				const std::string asciiHeader = "solid";

				// ascii 파일 읽기
				if (std::string(buf) == asciiHeader) {
					input_file.seekg(0, std::ios::beg);
					std::string s0, s1;
					while (!input_file.eof()) {
						input_file >> s0;
						if (s0 == "facet") {
							double x_temp, y_temp, z_temp;
							double x_avg = 0.0, y_avg = 0.0, z_avg = 0.0;
							input_file >> s1 >> x_temp >> y_temp >> z_temp;
							input_file >> s0 >> s1;

							msh.f2v.push_back({});

							input_file >> s0 >> x_temp >> y_temp >> z_temp;
							msh.pos.push_back({ x_temp, y_temp, z_temp });
							msh.f2v.back().push_back(msh.pos.size() - 1);

							input_file >> s0 >> x_temp >> y_temp >> z_temp;
							msh.pos.push_back({ x_temp, y_temp, z_temp });
							msh.f2v.back().push_back(msh.pos.size() - 1);

							input_file >> s0 >> x_temp >> y_temp >> z_temp;
							msh.pos.push_back({ x_temp, y_temp, z_temp });
							msh.f2v.back().push_back(msh.pos.size() - 1);

							input_file >> s0;
							input_file >> s0;

						}
						else if (s0 == "endsolid") {
							break;
						}

					}
				}
				// binary 파일 읽기
				else {
					input_file.close();
					input_file.open(file_path, std::ios::in | std::ios::binary);

					// STL 헤더 정보 건너뛰기
					char header[80];
					input_file.read(header, 80);

					// 삼각형 면 개수 읽기
					uint32_t num_triangles = 0;
					input_file.read(reinterpret_cast<char*>(&num_triangles), sizeof(uint32_t));

					for (size_t idx = 0; idx < num_triangles; ++idx) {

						float normal_t[3], vertex1_t[3], vertex2_t[3], vertex3_t[3];
						uint16_t attribute;
						input_file.read(reinterpret_cast<char*>(normal_t), sizeof(float) * 3);
						input_file.read(reinterpret_cast<char*>(vertex1_t), sizeof(float) * 3);
						input_file.read(reinterpret_cast<char*>(vertex2_t), sizeof(float) * 3);
						input_file.read(reinterpret_cast<char*>(vertex3_t), sizeof(float) * 3);
						input_file.read(reinterpret_cast<char*>(&attribute), sizeof(uint16_t));

						std::array<double, 3> normal;
						std::array<double, 3> vertex1, vertex2, vertex3;
						for (size_t i = 0; i < 3; ++i) {
							normal[i] = static_cast<double>(normal_t[i]);
							vertex1[i] = static_cast<double>(vertex1_t[i]);
							vertex2[i] = static_cast<double>(vertex2_t[i]);
							vertex3[i] = static_cast<double>(vertex3_t[i]);
						}
						msh.f2v.push_back({});
						msh.pos.push_back({ vertex1[0], vertex1[1], vertex1[2] });
						msh.f2v.back().push_back(msh.pos.size() - 1);
						msh.pos.push_back({ vertex2[0], vertex2[1], vertex2[2] });
						msh.f2v.back().push_back(msh.pos.size() - 1);
						msh.pos.push_back({ vertex3[0], vertex3[1], vertex3[2] });
						msh.f2v.back().push_back(msh.pos.size() - 1);
					}
				}

				// 파일 닫기
				input_file.close();

				std::cout << "Complete Load STL mesh from " << fileName << std::endl;



			};

			return load_func;

		}
		save_t save(std::string filename) override {

			save_t save_func;

			save_func = [filename](mesh& mesh) {


			};

			std::cout << "Saving STL mesh to " << std::endl;

			return save_func;
		}
	};

	class obj_mesh_io : public mesh_io_base {
	public:
		load_t load(std::string fileName) override {

			load_t load_func;

			load_func = [fileName](mesh& msh) {

				std::cout << "Loading OBJ mesh from " << std::endl;


			};

			return load_func;



		}
		save_t save(std::string fileName) override {
			save_t save_func;

			save_func = [fileName](mesh& msh) {
				std::ofstream file(fileName);
				if (!file.is_open()) {
					std::cerr << "파일을 열 수 없습니다." << std::endl;
					return;
				}

				for (const auto& v : msh.pos) {
					file << "v " << v[0] << " " << v[1] << " " << v[2] << std::endl;
				}
				for (const auto& v : msh.f2v) {
					file << "f " << v[0] + 1 << " " << v[1] + 1 << " " << v[2] + 1 << std::endl;
				}

				file.close();
				std::cout << "파일이 성공적으로 저장되었습니다." << std::endl;

			};



			std::cout << "Saving OBJ mesh to " << std::endl;
			return save_func;
		}
	};






	class openfoam_mesh_io : public mesh_io_base {
	public:
		load_t load(std::string fileName) override {

			load_t load_func;

			load_func = [fileName](mesh& msh) {

				auto& pos = msh.pos;
				auto& f2v = msh.f2v;
				auto& f2c = msh.f2c;


				std::string gridFolderName = fileName;
				std::string pointsName = "points";
				std::string facesName = "faces";
				std::string ownerName = "owner";
				std::string neighbourName = "neighbour";
				std::string boundaryName = "boundary";

				std::ifstream inputFile;
				std::string openFileName;

				// points 읽기
				openFileName = gridFolderName + "/" + pointsName;
				inputFile.open(openFileName);
				if (inputFile.fail()) {
					std::cerr << "Unable to open file for reading : " << openFileName << std::endl;
					return;
				}

				std::string nextToken;
				bool startInput = false;
				while (getline(inputFile, nextToken)) {
					std::string asignToken;

					if (startInput) {
						if (asignToken.assign(nextToken, 0, 1) == ")") {
							break;
						}
						else {
							std::vector<double> xyz(3, 0.0);
							nextToken.erase(nextToken.find("("), 1);
							nextToken.erase(nextToken.find(")"), 1);
							std::stringstream sstream(nextToken);
							std::string word;
							char del = ' ';
							int num = 0;
							while (getline(sstream, word, del)) {
								xyz[num] = stold(word);
								++num;
							}
							pos.push_back({ xyz[0], xyz[1], xyz[2] });
						}
					}
					else {
						if (asignToken.assign(nextToken, 0, 1) == "(") {
							startInput = true;
						}
					}
				}
				inputFile.close();



				// faces 읽기
				openFileName = gridFolderName + "/" + facesName;
				inputFile.open(openFileName);
				if (inputFile.fail()) {
					std::cerr << "Unable to open file for reading : " << openFileName << std::endl;
					return;
				}
				startInput = false;
				bool continueInput = false;
				std::string saveToken;
				while (getline(inputFile, nextToken)) {
					std::string asignToken;

					if (startInput) {
						if (asignToken.assign(nextToken, 0, 1) == ")" && !continueInput) {
							break;
						}
						else {
							if (nextToken.size() == 1) continue;

							if (nextToken.find(")") == std::string::npos) {
								saveToken.append(" ");
								rtrim(nextToken);
								saveToken.append(nextToken);
								continueInput = true;
								continue;
							}
							saveToken.append(" ");
							rtrim(nextToken);
							saveToken.append(nextToken);

							saveToken.replace(saveToken.find("("), 1, " ");
							saveToken.replace(saveToken.find(")"), 1, " ");
							std::istringstream iss(saveToken);
							size_t tempint;
							iss >> tempint;

							f2v.push_back({});

							while (iss >> tempint) {
								f2v.back().push_back(tempint);
							}
							saveToken.clear();
							continueInput = false;
						}
					}
					else {
						if (asignToken.assign(nextToken, 0, 1) == "(") {
							startInput = true;
						}
					}
				}
				inputFile.close();



				// owner
				openFileName = gridFolderName + "/" + ownerName;
				inputFile.open(openFileName);
				if (inputFile.fail()) {
					std::cerr << "Unable to open file for reading : " << openFileName << std::endl;
					return;
				}
				int temp_num = 0;
				startInput = false;
				while (getline(inputFile, nextToken)) {
					std::string asignToken;

					if (startInput) {
						if (asignToken.assign(nextToken, 0, 1) == ")") {
							break;
						}
						else {
							std::istringstream iss(nextToken);
							size_t tempint;
							while (iss >> tempint) {
								f2c.push_back({ tempint });
								++temp_num;
							}
						}
					}
					else {
						if (asignToken.assign(nextToken, 0, 1) == "(") {
							startInput = true;
						}
					}
				}
				inputFile.close();




				// neighbour
				openFileName = gridFolderName + "/" + neighbourName;
				inputFile.open(openFileName);
				if (inputFile.fail()) {
					std::cerr << "Unable to open file for reading : " << openFileName << std::endl;
					return;
				}
				temp_num = 0;
				startInput = false;
				while (getline(inputFile, nextToken)) {
					std::string asignToken;

					if (startInput) {
						if (asignToken.assign(nextToken, 0, 1) == ")") {
							break;
						}
						else {
							std::istringstream iss(nextToken);
							int tempint;
							while (iss >> tempint) {
								if (tempint < 0) break;
								f2c[temp_num].push_back(tempint);
								// mesh.faces[temp_num].thereR = true;
								//mesh.faces[temp_num].setType(MASCH_Face_Types::INTERNAL);
								++temp_num;
							}
						}
					}
					else {
						if (asignToken.assign(nextToken, 0, 1) == "(") {
							startInput = true;
						}
					}
				}
				inputFile.close();



				// boundary
				openFileName = gridFolderName + "/" + boundaryName;
				inputFile.open(openFileName);
				if (inputFile.fail()) {
					std::cerr << "Unable to open file for reading : " << openFileName << std::endl;
					return;
				}
				std::vector<std::string> boundary_name;
				std::vector<char> boundary_type;
				std::vector<int> boundary_nFaces;
				std::vector<int> boundary_startFace;
				std::vector<int> boundary_myProcNo;
				std::vector<int> boundary_neighbProcNo;

				std::string backToken;
				startInput = false;
				std::vector<std::string> setToken;
				while (getline(inputFile, nextToken)) {
					std::string asignToken;
					if (startInput) {
						if (asignToken.assign(nextToken, 0, 1) == ")") {
							break;
						}
						setToken.push_back(nextToken.c_str());
					}
					else {
						if (asignToken.assign(nextToken, 0, 1) == "(") {
							startInput = true;
						}
					}
					backToken = nextToken;
				}

				std::string names;
				std::vector<std::string> setToken2;
				startInput = false;
				for (auto item : setToken) {
					std::string asignToken;
					if (startInput) {
						if (item.find("}") != std::string::npos) {
							trim(names);

							boundary_name.push_back(names);
							int l = 0;
							for (auto item2 : setToken2) {
								if (item2.find("nFaces") != std::string::npos) {
									std::istringstream iss(item2);
									std::string temptemp;
									int temptempint;
									iss >> temptemp >> temptempint;
									boundary_nFaces.push_back(temptempint);
								}
								if (item2.find("startFace") != std::string::npos) {
									std::istringstream iss(item2);
									std::string temptemp;
									int temptempint;
									iss >> temptemp >> temptempint;
									boundary_startFace.push_back(temptempint);
								}
								if (item2.find("myProcNo") != std::string::npos) {
									std::istringstream iss(item2);
									std::string temptemp;
									int temptempint;
									iss >> temptemp >> temptempint;
									boundary_myProcNo.push_back(temptempint);
									++l;
								}
								if (item2.find("neighbProcNo") != std::string::npos) {
									std::istringstream iss(item2);
									std::string temptemp;
									int temptempint;
									iss >> temptemp >> temptempint;
									boundary_neighbProcNo.push_back(temptempint);
									++l;
								}
							}
							if (l == 0) {
								boundary_myProcNo.push_back(-1);
								boundary_neighbProcNo.push_back(-1);
							}

							startInput = false;
							setToken2.clear();
						}
						setToken2.push_back(item.c_str());
					}
					else {
						if (item.find("{") != std::string::npos) {
							names.clear();
							names = backToken;
							startInput = true;
						}
					}
					backToken = item;
				}



				inputFile.close();
				////mesh.boundaries.clear();
				//int nbcs = boundary_name.size();
				//for (int i = 0; i < boundary_name.size(); ++i) {
				//	mesh.addBoundary();
				//}
				//for (int i = 0; i < mesh.boundaries.size(); ++i) {
				//	mesh.boundaries[i].name = trim(boundary_name[i]);
				//	mesh.boundaries[i].nFaces = static_cast<int>(boundary_nFaces[i]);
				//	mesh.boundaries[i].startFace = static_cast<int>(boundary_startFace[i]);
				//	mesh.boundaries[i].myProcNo = static_cast<int>(boundary_myProcNo[i]);
				//	mesh.boundaries[i].rightProcNo = static_cast<int>(boundary_neighbProcNo[i]);
				//	// mesh.boundaries[i].thereR = false;
				//	mesh.boundaries[i].setType(MASCH_Face_Types::BOUNDARY);
				//}




				make_c2f_from_f2c(msh);
				make_c2v_from_c2f_f2v(msh);


				std::cout << "Loading OBJ mesh from " << std::endl;


			};

			return load_func;


		}
		save_t save(std::string fileName) override {
			save_t save_func;

			save_func = [fileName](mesh& msh) {
				std::ofstream file(fileName);
				if (!file.is_open()) {
					std::cerr << "파일을 열 수 없습니다." << std::endl;
					return;
				}

				for (const auto& v : msh.pos) {
					file << "v " << v[0] << " " << v[1] << " " << v[2] << std::endl;
				}
				for (const auto& v : msh.f2v) {
					file << "f " << v[0] + 1 << " " << v[1] + 1 << " " << v[2] + 1 << std::endl;
				}

				file.close();
				std::cout << "파일이 성공적으로 저장되었습니다." << std::endl;

			};



			std::cout << "Saving Openfoam mesh to " << std::endl;
			return save_func;
		}
	};









	class vtu_mesh_io : public mesh_io_base {
	public:
		load_t load(std::string fileName) override {

			load_t load_func;

			std::cout << "Loading OBJ mesh from " << std::endl;

			return load_func;
		}
		save_t save(std::string fileName) override {
			save_t save_func;



			save_func = [fileName](mesh& msh) {

				std::ofstream outputFile;
				std::string filenamePlot = fileName;

				outputFile.open(filenamePlot);
				if (outputFile.fail()) {
					std::cerr << "Unable to write file for writing." << std::endl;
					return;
				}

				using namespace std;

				// string out_line;
				outputFile << "<?xml version=\"1.0\"?>" << endl;
				outputFile << " <VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt64\">" << endl;
				outputFile << "  <UnstructuredGrid>" << endl;
				outputFile << "   <Piece NumberOfPoints=\"" << msh.pos.size() << "\" NumberOfCells=\"" << msh.c2f.size() << "\">" << endl;

				// Points data
				outputFile << "    <PointData>" << endl;
				outputFile << "    </PointData>" << endl;
				// Cells data
				outputFile << "    <CellData>" << endl;
				outputFile << "    </CellData>" << endl;
				// Points
				outputFile << "    <Points>" << endl;
				// }
				outputFile << "     <DataArray type=\"Float64\" Name=\"NodeCoordinates\" NumberOfComponents=\"3\" format=\"ascii\">" << endl;

				stringstream streamXYZ;
				outputFile.precision(20);
				// for(auto iter=mesh.points.begin(); iter!=mesh.points.end(); iter++){
				for (auto& point : msh.pos) {
					outputFile << scientific << point[0] << " " << point[1] << " " << point[2] << endl;

				}

				outputFile << "    </DataArray>" << endl;
				outputFile << "   </Points>" << endl;

				// cells
				outputFile << "   <Cells>" << endl;
				// connectivity (cell's points)
				outputFile << "    <DataArray type=\"Int64\" Name=\"connectivity\" format=\"ascii\">" << endl;

				for (auto& cell : msh.c2v) {
					for (auto i : cell) {
						outputFile << i << " ";
					}
					outputFile << endl;
				}

				outputFile << "    </DataArray>" << endl;

				// offsets (cell's points offset)
				int cellFaceOffset = 0;
				outputFile << "    <DataArray type=\"Int64\" Name=\"offsets\" format=\"ascii\">" << endl;

				cellFaceOffset = 0;
				for (auto& cell : msh.c2v) {
					cellFaceOffset += cell.size();
					outputFile << cellFaceOffset << " ";
				}
				outputFile << endl;

				outputFile << "    </DataArray>" << endl;

				// types (cell's type, 42 = polyhedron)
				outputFile << "    <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" << endl;

				for (auto& cell : msh.c2v) {
					outputFile << "42" << " ";
				}
				outputFile << endl;

				outputFile << "    </DataArray>" << endl;

				{
					// faces (cell's faces number, each face's point number, cell's faces's points)
					outputFile << "    <DataArray type=\"Int64\" IdType=\"1\" Name=\"faces\" format=\"ascii\">" << endl;

					for (auto& cell : msh.c2f) {
						outputFile << cell.size() << endl;
						for (auto& i : cell) {
							outputFile << msh.f2v[i].size() << " ";
							int tmp_space = 0;
							for (auto& j : msh.f2v[i]) {
								if (tmp_space != msh.f2v[i].size() - 1) {
									outputFile << j << " ";
								}
								else {
									outputFile << j;
								}
								++tmp_space;
							}
							outputFile << endl;
						}
					}

					outputFile << "    </DataArray>" << endl;


				}

				int cellFacePointOffset = 0;

				outputFile << "    <DataArray type=\"Int64\" IdType=\"1\" Name=\"faceoffsets\" format=\"ascii\">" << endl;

				cellFacePointOffset = 0;
				for (auto& face : msh.c2f) {
					int numbering = 1 + face.size();
					for (auto& i : face) {
						numbering += msh.f2v[i].size();
					}
					cellFacePointOffset += numbering;
					outputFile << cellFacePointOffset << " ";
				}
				outputFile << endl;

				outputFile << "    </DataArray>" << endl;
				outputFile << "   </Cells>" << endl;


				outputFile << "  </Piece>" << endl;
				outputFile << " </UnstructuredGrid>" << endl;





				//// additional informations
				//{
				//	string saveFormat = "ascii";
				//	outputFile << " <DataArray type=\"Int32\" Name=\"owner\" format=\"" << saveFormat << "\">" << endl;
				//	// vector<int> values;
				//	for (auto& face : mesh.faces) {
				//		// values.push_back(face.owner);
				//		outputFile << face.iL << " ";
				//	}
				//	// writeDatasAtVTU(controls, outputFile, values);
				//	outputFile << endl;
				//	outputFile << " </DataArray>" << endl;
				//}
				//{
				//	string saveFormat = "ascii";
				//	outputFile << " <DataArray type=\"Int32\" Name=\"neighbour\" format=\"" << saveFormat << "\">" << endl;
				//	// vector<int> values;
				//	for (auto& face : mesh.faces) {
				//		// values.push_back(face.neighbour);
				//		if (face.getType() == MASCH_Face_Types::INTERNAL) {
				//			outputFile << face.iR << " ";
				//		}
				//	}
				//	// writeDatasAtVTU(controls, outputFile, values);
				//	outputFile << endl;
				//	outputFile << " </DataArray>" << endl;
				//}



				//// boundary informations
				//{
				//	outputFile << " <DataArray type=\"Char\" Name=\"bcName\" format=\"" << "ascii" << "\">" << endl;
				//	// for(auto& boundary : mesh.boundary){
				//		// // cout << boundary.name << endl;
				//		// // trim;
				//		// string bcName = boundary.name;

				//		// bcName.erase(std::find_if(bcName.rbegin(), bcName.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), bcName.end());
				//		// bcName.erase(bcName.begin(), std::find_if(bcName.begin(), bcName.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));


				//		// // outputFile << boundary.name << " ";
				//		// outputFile << bcName << " ";
				//	// }
				//	for (auto& boundary : mesh.boundaries) {
				//		// cout << boundary.name << endl;
				//		outputFile << boundary.name << " ";
				//	}
				//	outputFile << endl;
				//	outputFile << " </DataArray>" << endl;

				//	outputFile << " <DataArray type=\"Int32\" Name=\"bcStartFace\" format=\"" << "ascii" << "\">" << endl;
				//	for (auto& boundary : mesh.boundaries) {
				//		outputFile << boundary.startFace << " ";
				//	}
				//	outputFile << endl;
				//	outputFile << " </DataArray>" << endl;

				//	outputFile << " <DataArray type=\"Int32\" Name=\"bcNFaces\" format=\"" << "ascii" << "\">" << endl;
				//	for (auto& boundary : mesh.boundaries) {
				//		outputFile << boundary.nFaces << " ";
				//	}
				//	outputFile << endl;
				//	outputFile << " </DataArray>" << endl;

				//	outputFile << " <DataArray type=\"Int32\" Name=\"bcNeighbProcNo\" format=\"" << "ascii" << "\">" << endl;
				//	for (auto& boundary : mesh.boundaries) {
				//		// cout << boundary.rightProcNo << endl;
				//		outputFile << boundary.rightProcNo << " ";
				//	}
				//	outputFile << endl;
				//	outputFile << " </DataArray>" << endl;

				//	outputFile << " <DataArray type=\"Int32\" Name=\"connPoints\" format=\"" << "ascii" << "\">" << endl;
				//	for (int i = 0; i < mesh.points.size(); ++i) {
				//		auto& point = mesh.points[i];
				//		for (auto& [item, item2] : point.connPoints) {
				//			outputFile << i << " " << item << " " << item2 << " ";
				//		}
				//		// if(!point.connPoints.empty()) outputFile << endl;
				//	}
				//	outputFile << endl;
				//	outputFile << " </DataArray>" << endl;


				//}

				outputFile << "</VTKFile>" << endl;

				outputFile.close();

				// }









				std::cout << "파일이 성공적으로 저장되었습니다." << std::endl;

				};



			std::cout << "Saving OBJ mesh to " << std::endl;
			return save_func;
		}
	};







	class mesh_io_factory {
	public:
		mesh_io_factory() {
			creators["stl"] = []() { return std::make_unique<stl_mesh_io>(); };
			creators["obj"] = []() { return std::make_unique<obj_mesh_io>(); };
			creators["vtu"] = []() { return std::make_unique<vtu_mesh_io>(); };
			creators[""] = []() { return std::make_unique<openfoam_mesh_io>(); };
		}

		std::unique_ptr<mesh_io_base> create(const std::string& format) {
			auto it = creators.find(format);
			if (it != creators.end()) {
				return it->second();
			}
			else {
				return nullptr;
			}
		}

	private:
		std::map<std::string, std::function<std::unique_ptr<mesh_io_base>()>> creators;
	};





	class mesh_io {
	public:
		std::optional<load_t> load(const std::string& filename) {
			std::string format = get_file_extension(filename);
			to_lower(format);
			mesh_io_ = std::make_unique<mesh_io_factory>()->create(format);
			if (!mesh_io_) {
				std::cerr << "Unknown format: " << format << std::endl;
				return std::nullopt;
			}
			return mesh_io_->load(filename);
		}

		std::optional<save_t> save(const std::string& filename) {
			std::string format = get_file_extension(filename);
			to_lower(format);
			mesh_io_ = std::make_unique<mesh_io_factory>()->create(format);
			if (!mesh_io_) {
				std::cerr << "Unknown format: " << format << std::endl;
				return std::nullopt;
			}
			return mesh_io_->save(filename);
		}

	private:
		std::unique_ptr<mesh_io_base> mesh_io_;
	};





	void make_c2f_from_f2c(mesh& msh) {

		if (msh.f2c.size() == 0) {
			std::cerr << "f2c is empty" << std::endl;
			return;
		}

		size_t num_cell = 0;
		for (auto& cs : msh.f2c) {
			for (auto& c : cs) {
				if(c > num_cell) num_cell = c;
			}
		}
		num_cell += 1;
		msh.c2f.resize(num_cell);
		for (size_t f = 0; auto & cs : msh.f2c) {
			for (auto& c : cs) {
				msh.c2f[c].push_back(f);
			}
			++f;
		}

	}


	void make_c2v_from_c2f_f2v(mesh& msh) {

		if (msh.c2f.size() == 0) {
			std::cerr << "c2f is empty" << std::endl;
			return;
		}
		if (msh.f2v.size() == 0) {
			std::cerr << "f2v is empty" << std::endl;
			return;
		}

		msh.c2v.resize(msh.c2f.size());
		for (size_t i = 0; auto & fs : msh.c2f) {
			auto& c2v_r = msh.c2v[i];
			for (auto& f : fs) {
				for (auto& v : msh.f2v[f]) {
					c2v_r.push_back(v);
				}
			}
			std::sort(c2v_r.begin(), c2v_r.end());
			std::unique(c2v_r.begin(), c2v_r.end());
			++i;
		}

	}



	class mesh_treatment {
	public:





	};



	mesh& unique_vertex(mesh& msh) {






		return msh;
	}


	mesh& unique_face(mesh& msh) {






		return msh;
	}


	mesh& unique_cell(mesh& msh) {






		return msh;
	}






};
