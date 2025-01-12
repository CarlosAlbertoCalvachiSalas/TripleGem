// Adapted from Carlos Barreto Bachelor's thesis code

#include <fstream>
#include <cmath>
#include <regex>

#include <TCanvas.h>
#include <TApplication.h>

#include "Garfield/MediumMagboltz.hh"
#include "Garfield/ComponentElmer.hh"
#include "Garfield/AvalancheMicroscopic.hh"
#include "Garfield/ViewFEMesh.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/Sensor.hh"

using namespace Garfield;
using namespace std;

int main(int argc, char *argv[]) {
	
	TApplication app("app", &argc, argv);

	// Set relevant GEM parameters.
	const double gem_th = 0.0050;       // GEM thickness in cm    
	const double gem_cpth = 0.0005;     // Copper thickness
	const double gem_pitch = 0.0140;    // GEM pitch in cm
	const double axis_x = 0.08;         // X-width of drift simulation will cover between +/- axis_x
	const double axis_y = 0.3;          // Y-width of drift simulation will cover between +/- axis_y  
	const double axis_z = 0.303;        // Z-width of drift simulation will cover between +/- axis_z  

	// Initial position of the incoming particle
	const double zi = 0.302;
	double xi = 0.;
	double yi = 0.;

	double dxi = 0.0;
	double dyi = 0.0;
	double dzi = -1.0;

	// Number of events
	int events = 100;

	int voltage = 400;
	// Energy of the incoming particle in eV
	int intEnergy = 9;

	double energy = pow(10, intEnergy);

	int argon = 70;

	int co2 = 100 - argon;

	if(argc > 1){

		for (int k = 1; k < argc; ++k)
		{	
			regex rgx("(\\D+)=(\\S+)+");

			cmatch partsCommands; 

			if(regex_match(argv[k], partsCommands, rgx)){

				if(partsCommands[1] == "x" || partsCommands[1] == "X"){
					xi = stod(partsCommands[2]);
				}else if(partsCommands[1] == "y" || partsCommands[1] == "Y"){
					yi = stod(partsCommands[2]);
				}else if(partsCommands[1] == "dx" || partsCommands[1] == "Dx" || partsCommands[1] == "DX" || partsCommands[1] == "dX"){
					dxi = stod(partsCommands[2]);
				}else if(partsCommands[1] == "dy" || partsCommands[1] == "Dy" || partsCommands[1] == "DY" || partsCommands[1] == "dY"){
					dyi = stod(partsCommands[2]);
				}else if(partsCommands[1] == "N"|| partsCommands[1] == "n"|| partsCommands[1] == "event" || partsCommands[1] == "events"){
					events = stoi(partsCommands[2]);
				}else if(partsCommands[1] == "intEnergy") {
					intEnergy = stoi(partsCommands[2]);
					energy = pow(10, intEnergy);
				}else if(partsCommands[1] == "Energy" || partsCommands[1] == "energy" || partsCommands[1] == "e"|| partsCommands[1] == "E"){
					energy = stod(partsCommands[2]);
				}else if(partsCommands[1] == "argon" || partsCommands[1] == "ARGON" || partsCommands[1] == "Ar" || partsCommands[1] == "AR"){
					argon = stod(partsCommands[2]);
					co2 = 100 - argon;
				}

			}
			
		} 

	} else {

		cout << "Energy(eV): 1E ";
		cin >> intEnergy;
		energy = pow(10, intEnergy);

		// Define gaseous medium

		cout << "Argon: ";
		cin >> argon;
		co2 = 100 - argon;

	}

	// Grafical interface for the plots
	TCanvas *cGeom = new TCanvas("Track", "Avalancha por muon que ingresa al detector");
	cGeom->SetLeftMargin(0.14);



	// Create files to save simulation results 

	ofstream dataElectrons;
	ofstream dataAvalanches;

	dataElectrons.open("dataElectronsAr"  + to_string(argon) + "E" + to_string(intEnergy)  + "V" + to_string(voltage) + ".csv");
	dataAvalanches.open("dataAvalanchesAr" + to_string(argon) + "E" + to_string(intEnergy) + "V" + to_string(voltage) + ".csv");

	dataElectrons << "Ar" << ","  << "E" << "," << "V" << "," << "step" << "," << "elecNum" << "," << "x" << "," << "y" << "," << "z" <<  "," <<  "t" << "," <<  "e"  <<"\n" ;
	dataAvalanches << "Ar"<< ","  << "E" << "," << "V" << "," << "step" <<  "," << "avalTot" << "," << "filTot" << ","  << "ne" << "," << "nIons" << "\n" ;

	MediumMagboltz *gas = new MediumMagboltz();
	gas->SetTemperature(293.15);	// Set the temperature (K)
	gas->SetPressure(760.);			// Set the pressure(Torr)
	gas->EnableDrift();				// Allow for drifting in this medium
	gas->SetComposition("ar", argon, "co2", co2);	// Specify the gas mixture (Ar/CO2 70:30)
	gas->SetMaxElectronEnergy(200.);
	// Import Elmer-created field map
	ComponentElmer *elm = new ComponentElmer(
      "gemcell/mesh.header", "gemcell/mesh.elements", "gemcell/mesh.nodes",
      "gemcell/dielectrics.dat", "gemcell/gemcell.result", "cm");
	elm->EnablePeriodicityX();
	elm->EnableMirrorPeriodicityY();
	elm->SetMedium(0, gas);

	// Set up a sensor object.
	Sensor *sensor = new Sensor();
	sensor->AddComponent(elm);
	sensor->SetArea(-axis_x, -axis_y, -axis_z, axis_x, axis_y, axis_z);

	// Set up object for drift line visualization
	ViewDrift *viewDrift = new ViewDrift();
	viewDrift->SetArea(-axis_x, -axis_y, -axis_z, axis_x, axis_y, axis_z);
	viewDrift->SetCanvas(cGeom);

	// Set up object that represents the avalanche of electrons
	AvalancheMicroscopic *aval = new AvalancheMicroscopic();
	aval->SetSensor(sensor);
	aval->SetCollisionSteps(100);
	aval->EnablePlotting(viewDrift);
  
	// Set up object for the simulation of the incoming particle
	TrackHeed *track = new TrackHeed();
	track->SetParticle("mu-");
	track->SetEnergy(energy);
	track->SetSensor(sensor);
	track->EnableMagneticField();
	track->EnableElectricField();
	track->EnablePlotting(viewDrift);
	track->EnableDebugging();

	// Coordenadas de la colision inicial
	double track_x = xi; // [cm]
	double track_y = yi;
	double track_z = zi;
    
	// Componentes de la velocidad inicial de la particula incidente
	double track_dx = dxi;
	double track_dy = dyi;
	double track_dz = dzi;

	// Loop over all incoming particles
	for(int i = 0; i < events; ++i){
		track->NewTrack(track_x, track_y, track_z, 100*i, track_dx, track_dy, track_dz);
		double xc = 0., yc = 0., zc = 0., tc = 0., ec = 0., extra = 0.;
		int nc = 0;
		int contador = 0;
		// Loop over each cluster generated by an incoming particle
		while(track->GetCluster(xc, yc, zc, tc, nc, ec, extra)){
			// Loop over all electrons generated in each cluster
			for(int j = 0; j < nc; ++j) {
				double xe = 0., ye = 0., ze = 0., te = 0., ee = 0.;
				double dxe = 0., dye = 0., dze = 0.;
				track->GetElectron(j, xe, ye, ze, te, ee, dxe, dye, dze);
				aval->AvalancheElectron(xe, ye, ze, te, ee, dxe, dye, dze);
				
				int nAvalanche = aval->GetNumberOfElectronEndpoints();

				double xe1, ye1, ze1, te1, e1;
				double xe2, ye2, ze2, te2, e2;
				int status;

				//cout << j + 1 << "/" << nc;
				//cout << "... avalanche complete with "
				//	<< nAvalanche << " electron tracks.\n";
				
				int filteredElectrons = 0;

				for(int k = nAvalanche; k>0; k--){
					aval->GetElectronEndpoint(k-1,xe1,ye1,ze1,te1,e1,xe2,ye2,ze2,te2,e2,status);
					//printf("%d,%f,%f,\n", nAvalanche - k, xe2, ye2);
					//cout << "ze2 = " << ze2 << endl;
					

					if(ze2 <= -0.2029){
						filteredElectrons += 1;
						
					//	printf("No. de electrones que han llegado al electrodo de lectura %d\n", nReadOutElectrons);
					//	ddata_elec_read << xe2 << "\t" << ye2 << "\t" << ze2 << "\n";
					}

					dataElectrons  << argon << ","  <<  intEnergy << ","  << voltage << "," << i + 1 << "," << (nAvalanche - k)+1 << "," << xe2 << "," << ye2 << "," << ze2 <<  "," <<  te2 << "," <<  e2  << "\n" ;

				}

				int ne, ni;
				
				aval->GetAvalancheSize(ne, ni);
				//printf("ne = %d   ni = %d\n", ne, ni);

				dataAvalanches  << argon << ","  <<  intEnergy << ","  << voltage << "," << i + 1 <<  "," << nAvalanche << "," << filteredElectrons << ","  << ne << "," << ni << "\n" ;

			}
			contador++;
		}

		cout << i + 1 << "/" << events << "\t" << "clusters" << "\t" << contador  << "\n"; 
		//printf("Numero de clusters = %d\n", contador);
	
	}

	dataElectrons.close();
	dataAvalanches.close();

	/*ViewFEMesh * vFE = new ViewFEMesh();
	vFE->SetArea(-axis_x, -axis_y, -0.7*axis_z, axis_x, axis_y, axis_z);
	vFE->SetCanvas(cGeom);
	vFE->SetComponent(elm);
	vFE->SetPlane(0, -1, 0, 0, 0, 0);
	vFE->SetFillMesh(true);
	vFE->SetColor(1, kGray);
	vFE->SetColor(2, kYellow + 3);
	vFE->SetColor(3, kYellow + 3);
	vFE->EnableAxes();
	vFE->SetViewDrift(viewDrift);
	vFE->SetXaxisTitle("x(cm)");
	vFE->SetYaxisTitle("z(cm)");
	vFE->Plot();
 	*/

	//app.Run(kTRUE);
  	cout << "FIN SIMULACION" << endl; 
 	return 0;
}
