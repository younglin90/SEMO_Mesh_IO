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
#include <numeric>

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





	//===================================================
	// 요소들이 거의 동등한 값인지 확인하는 함수
	template <typename Container>
	bool approximately_equal(const Container& a, const Container& b,
		const typename Container::value_type& epsilon =
		2.0 * std::numeric_limits<typename Container::value_type>::epsilon()) {
		auto it_a = a.begin();
		auto it_b = b.begin();

		while (it_a != a.end() && it_b != b.end()) {
			if (std::abs(*it_a - *it_b) > epsilon) {
				return false;
			}
			++it_a;
			++it_b;
		}

		return true;
	}


	//===================================================
	// 주어진 컨테이너의 값을 "정렬"과 "중복 제거"한 후,
	// 제거된 빈 공간을 모두 앞으로 땡겨서 채운 후, 
	// 각 원소의 새로운 인덱스(중복이면 중복 요소 중 
	// 가장 첫번째 요소의 인덱스) 를 반환합니다.
	template <typename Container, typename Func =
		std::function<typename Container::value_type& (typename Container::value_type&)>>
		std::vector<std::size_t> sort_and_unique_indices(
			Container& values,
			Func func = [](auto& p) ->
			auto& {
				return p;
			}) {
		//template <typename Container, typename Func>
		//std::vector<unsigned> sortAndUniqueIndices(Container & values, Func func) {
			//std::function<T> func = [](typename Container::value_type& p) { return p; }) {

		std::vector<std::size_t> indices(values.size());
		std::vector<std::size_t*> indicesPtr(values.size());
		for (std::size_t i = 0; i < values.size(); ++i) {
			indices[i] = i;
			indicesPtr[i] = &indices[i];
		}
		//std::iota(std::begin(indices), std::end(indices), 0);

		// 주어진 함수를 이용하여 값을 비교하여 인덱스를 정렬합니다.
		std::sort(indicesPtr.begin(), indicesPtr.end(),
			[&values, func](std::size_t* i1, std::size_t* i2) {
				return std::lexicographical_compare(
					func(values[*i1]).begin(), func(values[*i1]).end(),
					func(values[*i2]).begin(), func(values[*i2]).end());
			});

		// 정렬된 인덱스를 다시 부여합니다.
		for (std::size_t i = 0; i < values.size(); ++i) {
			*indicesPtr[i] = static_cast<std::size_t>(i);
		}

		// 주어진 함수를 이용하여 값을 비교하여 원소를 정렬합니다.
		std::sort(values.begin(), values.end(), [func](auto& a, auto& b) {
			return std::lexicographical_compare(
				func(a).begin(), func(a).end(),
				func(b).begin(), func(b).end());
			});


		auto approximately_equal_with_epsilon = [](auto a, auto b) {
			return approximately_equal(a, b, 1.e-12);
			};

		// 중복된 원소에 대한 새로운 인덱스를 부여합니다.
		for (std::size_t i = 1, j = 0; i < values.size(); ++i) {
			if (approximately_equal_with_epsilon(func(values[i - 1]), func(values[i]))) {
				//indicesPtr[i]->second = true;
				*indicesPtr[i] = static_cast<std::size_t>(j);
			}
			else {
				*indicesPtr[i] = static_cast<std::size_t>(++j);
			}
		}

		// 중복된 원소들을 제거합니다.
		auto new_end = std::unique(values.begin(), values.end(),
			[approximately_equal_with_epsilon, func](auto& a, auto& b) {
				return approximately_equal_with_epsilon(func(a), func(b));
			});

		values.erase(new_end, values.end());

		return indices;

	}


	////===================================================
	//// 벡터를 플랫하게 만들어주는 함수
	//template <typename E, typename X>
	//constexpr void flatten(const std::vector<E>& v, std::vector<X>& out) {
	//	//std::cout << "unroll vector\n";
	//	out.insert(out.end(), v.begin(), v.end());
	//}
	//// 벡터를 플랫하게 만들어주는 함수
	//template <typename V, typename X>
	//constexpr void flatten(const std::vector<std::vector<V>>& v, std::vector<X>& out) {
	//	//std::cout << "unroll vector of vectors\n";
	//	for (const auto& e : v) flatten(e, out);
	//}
	//// 벡터를 플랫하게 만들어주는 함수
	//template<typename T>
	//constexpr auto flatten(const T& input)
	//{
	//	using NestedType = semo::types::nested_type_t<T>;
	//	std::vector<NestedType> flattened;
	//	flatten(input, flattened);
	//	return flattened;
	//}



	//===================================================
	// 최대 최소 요소 리턴
	template<typename Out, typename T, typename Func>
	std::pair< Out, Out> minmax_elements(T& container, Func func) {
		auto minmax = std::minmax_element(
			std::begin(container), std::end(container),
			[func](auto& elem1, auto& elem2) {
				return func(elem1) < func(elem2); // Added return statement
			});
		return std::make_pair(
			func(*(minmax.first)),
			func(*(minmax.second))
		);
	}

	template<typename Out, typename T>
	std::pair< Out, Out> minmax_elements(T& container) {
		return minmax_elements<Out>(container,
			[](const auto& elem) { return elem; });
	}




	//===================================================
	// myVector 요소들을 indices 로 옮기기
	template<typename T>
	void rearrange_elements_to_indices_mutable(std::vector<T>& myVector,
		std::vector<std::size_t>& indices) {
		std::size_t size = myVector.size();

		// 각 인덱스에 대해 현재 위치의 요소를 목표 위치로 swap합니다.
		for (std::size_t i = 0; i < size; ++i) {
			while (indices[i] != i) {
				std::swap(myVector[i], myVector[indices[i]]);
				std::swap(indices[i], indices[indices[i]]);
			}
		}
	}

	// myVector 요소들을 indices 로 옮기기
	template<typename T>
	void rearrange_elements_to_indices(std::vector<T>& myVector,
		std::vector<std::size_t>& indices) {
		std::vector<std::size_t> copy_indices(indices);
		rearrange_elements_to_indices_mutable(myVector, copy_indices);
	}

	// indices 위치에 myVector 요소를 옮기기
	template<typename T>
	void rearrange_elements_from_indices(std::vector<T>& myVector,
		std::vector<std::size_t>& indices) {
		std::size_t size = myVector.size();
		std::vector<std::size_t> copy_indices(size);
		for (std::size_t i = 0; i < size; ++i) {
			copy_indices[indices[i]] = i;
		}
		rearrange_elements_to_indices_mutable(myVector, copy_indices);
	}



	//===================================================
	// 함수형 프로그래밍
	template <typename F>
	auto compose(F&& f)
	{
		return [a = std::move(f)](auto&&... args) {
			return a(std::move(args)...);
			};
	}

	template <typename F1, typename F2, typename... Fs>
	auto compose(F1&& f1, F2&& f2, Fs&&... fs)
	{
		return compose(
			[first = std::move(f1), second = std::move(f2)]
			(auto&&... args) {
				return second(first(std::move(args)...));
			},
			std::move(fs)...
		);
	}


	template <typename... Funcs>
	decltype(auto) combine(Funcs... fs) {
		return [fs...](auto... inps) {
			(fs(inps...), ...);
			};
	}


	//===================================================
	// 있는지 확인
	template <typename Container, typename Value>
	bool is_there(Container& inp, Value value) {
		return std::find(inp.begin(), inp.end(), value) != inp.end();
	}

	//===================================================
	// input 으로 인덱스가 모여져 있는 벡터를 넣으면, 인덱스 부분 한꺼번에 효율적으로 제거
	template<typename T, typename Container2>
	void erase_elements_by_indices(std::vector<T>& myVector,
		const Container2& indices) {
		std::size_t size = myVector.size();
		std::vector<bool> to_remove(size, false);
		for (auto i : indices) {
			to_remove[i] = true;
		}
		std::size_t i = 0;
		myVector.erase(
			std::remove_if(myVector.begin(), myVector.end(),
				[&to_remove, i](const auto&) mutable {
					return to_remove[i++];
				}),
			myVector.end());
	}




	mesh& unique_vertex(mesh& msh) {

		size_t org_size = msh.pos.size();
		auto indices = sort_and_unique_indices(msh.pos);
		size_t new_size = msh.pos.size();
		std::cout << "기존 vertex 사이즈 " << org_size << std::endl;
		std::cout << "새로운 vertex 사이즈 " << new_size << std::endl;
		std::cout << "중복된 vertex 사이즈 " << org_size - new_size << std::endl;

		// face to point connectivity를 새로운 인덱스로 바꾸기
		for (auto& points : msh.f2v) {
			std::vector<std::size_t> new_points;
			for (const auto& point : points) {
				new_points.push_back(indices[point]);
			}
			points = std::move(new_points);
		}

		return msh;
	}


	mesh& unique_face(mesh& msh) {






		return msh;
	}


	mesh& unique_cell(mesh& msh) {






		return msh;
	}




};
