/**
   reader.cpp reads and initializes the scene

   @author George Tang
 */

/*
   @param scene name of scene to be created
 */
void init(string scene) {
	ifstream reader(scene+"/master.txt");
	string input;
	vector<string> files;
	unordered_map<string, string> tarr;
	while(getline(reader, input)) {
		vector<string> data;
		boost::split(data, input, boost::is_any_of(" "), boost::token_compress_on);
		char type = data[0][0];
		if (type == '#')
			continue;
		else if (type == '%')         // force quit
			break;
		else if (type == 'C') {
			double ud = 0.01745329251;
			eye = Vec3d(stod(data[1]), stod(data[2]), stod(data[3]));
			step = stod(data[4]);
			astep = ud*stod(data[5]);
			angle = ud*stod(data[6]);
			zoom = stod(data[7]);
		}
		else if (type == 'T') {
			tarr[data[1]] = data[2];
		}
		else if (type == 'L') {
			light.push_back(Light(stod(data[8]),
			                      Vec3d(stod(data[2]), stod(data[3]), stod(data[4])),
			                      Vec3d(stod(data[5]), stod(data[6]), stod(data[7])),
			                      data[1][0]));
		}
		else if (type == 'O') {
			files.push_back(data[1]);
		}
	}
	// OBJ File Reader
	for (int i = 0; i < files.size(); i++) {
		PolygonMesh mesh(Vec3d(1));
		ifstream meshReader(scene+"/"+files[i]);
		string line;
		int tcnt = 0;
		int tval;
		int reft = 0;
		while(getline(meshReader, line)) {
			vector<string> tkns;
			boost::split(tkns, line, boost::is_any_of(" "), boost::token_compress_on);
			char type = tkns[0][tkns[0].size()-1];
			switch(type) {
				case '%': {
					tcnt = -99999;
					break;
				}
				case 'g': {
					reft = 0;
					tval = -99999;
					if (tkns.size() == 1)
						break;
					if (tarr.find(tkns[1]) != tarr.end()) {
						cv::Mat tmap = cv::imread(scene+"/"+tarr[tkns[1]]+".jpg", cv::IMREAD_COLOR);
						mesh.tmaps.push_back(tmap);
						tval = tcnt++;
						// interpolate by point
						if (tkns.size() > 2 && tkns[2][0] == '?')
							tval = -tval-1;
					}
					if (tkns.size() > 2) {
						if (tkns[2][0] == '1')
							reft = 1;
					}
					break;
				}
				case 'v': {
					mesh.vert.push_back(Vec3d(stod(tkns[1]), stod(tkns[2]), stod(tkns[3])));
					break;
				}
				case 'n': {
					mesh.norm.push_back(Vec3d(stod(tkns[1]), stod(tkns[2]), stod(tkns[3])));
					break;
				}
				case 't': {
					mesh.text.push_back(pdd( max(0.0, min(0.99, stod(tkns[1]))), max(0.0, min(0.99, stod(tkns[2]))) ));
					break;
				}
				case 'f': {
					vector<int> data;
					int num;
					int sz = tkns.size() - 1;
					for (int j = 1; j <= sz; j++) {
						vector<string> spc;
						boost::split(spc, tkns[j], boost::is_any_of("/"), boost::token_compress_on);
						for (int k = 0; k < spc.size(); k++)
							data.push_back(stoi(spc[k]));
						num = spc.size();
					}
					// face, normal, texture
					for (int j = 1; j < sz-1; j++) {
						Triangle tri(Vec3i(data[0]-1, data[j*num]-1, data[(j+1)*num]-1), Vec3i(-1), Vec3i(-1), tval, reft);
						if (num > 2) {
							tri.ti = Vec3i(data[1]-1, data[j*num+1]-1, data[(j+1)*num+1]-1);
							tri.ni = Vec3i(data[2]-1, data[j*num+2]-1, data[(j+1)*num+2]-1);
						}
						else
							tri.ni = Vec3i(data[1]-1, data[j*num+1]-1, data[(j+1)*num+1]-1);
						mesh.tris.push_back(tri);
						mesh.cent.push_back((mesh.vert[tri.vi.x]+mesh.vert[tri.vi.y]+mesh.vert[tri.vi.z])/3.0);
					}
					break;
				}
				default: {
					continue;
				}
			}
			if (tcnt == -99999)             // continue force quit
				break;
		}
		obj.push_back(mesh);
	}
}
