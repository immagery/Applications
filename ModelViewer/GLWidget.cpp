#include "GLWidget.h"

#include <ui_mainwindow.h>

#include <Computation\BiharmonicDistances.h>
#include <Computation\mvc_interiorDistances.h>
#include <DataStructures\InteriorDistancesData.h>
#include <utils/util.h>

#include <QtCore/QTextStream>
#include <QtCore/qdir.h>

#include <tetgen/tetgen.h>

#include <render/graphRender.h>
#include <computation/HarmonicCoords.h>

#include <render/defGorupRender.h>

#include <Computation\AirSegmentation.h>


#define ratioExpansion_DEF 0.7


GLWidget::GLWidget(QWidget * parent, const QGLWidget * shareWidget,
                   Qt::WindowFlags flags ) : AdriViewer(parent, shareWidget, flags) {
    drawCage = true;
    shadeCoordInfluence = true;
    bGCcomputed = false;
    bHComputed = false;

    //m_bHCGrid = false;

    bGCcomputed = false;
    bHComputed = false;
    bMVCComputed = false;

    influenceDrawingIdx = 0;

    //activeCoords = HARMONIC_COORDS;
    //m.sModelPath = "";

    stillCageAbled = false;
    stillCageSelected = -1;

	valueAux = 0;

	if(escena->rig)
		delete escena->rig;

	escena->rig = new AirRig(scene::getNewId());

}

void GLWidget::doTests(string fileName, string name, string path) 
{

	assert(false);
	// To fix for the latest structure changes [13/11/2013]

/*
            getStatisticsFromData(fileName, name, path);
            return;

            // Realizaremos los tests especificados en un fichero.
            QFile file(QString(fileName.c_str()));

            if(!file.open(QFile::ReadOnly)) return;

            QTextStream in(&file);

            QString resultFileName = in.readLine();
            QString workingPath = in.readLine();

            QStringList globalCounts = in.readLine().split(" ");
            int modelCount = globalCounts[0].toInt();
            int interiorPointsCount = globalCounts[1].toInt();

            vector<QString> modelPaths(modelCount);
            vector<QString> modelSkeletonFile(modelCount);
            vector<QString> modelNames(modelCount);
            vector< vector<QString> >modelFiles(modelCount);
            vector< vector<Vector3d> >interiorPoints(modelCount);

            QStringList thresholdsString = in.readLine().split(" ");
            vector< double >thresholds(thresholdsString.size());
            for(int i = 0; i< thresholds.size(); i++)
                thresholds[i] = thresholdsString[i].toDouble();

            for(int i = 0; i< modelCount; i++)
            {
                modelNames[i] = in.readLine();
                modelPaths[i] = in.readLine();
                modelSkeletonFile[i] = in.readLine();
                modelFiles[i].resize(in.readLine().toInt());

                for(int j = 0; j < modelFiles[i].size(); j++)
                    modelFiles[i][j] = in.readLine();

                QStringList intPointsModel = in.readLine().split(" ");
                interiorPoints[i].resize(interiorPointsCount);

                assert(intPointsModel.size() == interiorPointsCount*3);

                for(int j = 0; j< intPointsModel.size(); j = j+3)
                {
                    double v1 = intPointsModel[j].toDouble();
                    double v2 = intPointsModel[j+1].toDouble();
                    double v3 = intPointsModel[j+2].toDouble();
                    interiorPoints[i][j/3] = Vector3d(v1, v2, v3);
                }
            }
            file.close();

            // Preparamos y lanzamos el calculo.
            QFile outFile(workingPath+"/"+resultFileName);
            if(!outFile.open(QFile::WriteOnly)) return;
            QTextStream out(&outFile);

            // time variables.
            float time_init = -1;
            float time_AComputation = -1;
            float time_MVC_sorting = -1;
            float time_precomputedDistance = -1;
            float time_finalDistance = -1;

            for(int i = 0; i< modelCount; i++)
            {
                for(int mf = 0; mf< modelFiles[i].size(); mf++)
                {
                    //Leemos el modelo
                    QString file = workingPath+"/"+modelPaths[i]+"/"+modelFiles[i][mf];
                    QString path = workingPath+"/"+modelPaths[i]+"/";
                    readModel( file.toStdString(), modelNames[i].toStdString(), path.toStdString());
                    Modelo* m = ((Modelo*)escena->models.back());

                    QString sktFile = workingPath+"/"+modelPaths[i]+"/"+modelSkeletonFile[i];

                    clock_t ini, fin;
                    ini = clock();

                    // Incializamos los datos
                    BuildSurfaceGraphs(m);

                    readSkeletons(sktFile.toStdString(), m->bind->bindedSkeletons);

                    for(int bindSkt = 0; bindSkt < m->bindings.size(); bindSkt++)
                    {
                        for(int bindSkt2 = 0; bindSkt2 < m->bindings[bindSkt]->bindedSkeletons.size(); bindSkt2++)
                        {
                            float minValue = GetMinSegmentLenght(getMinSkeletonSize(m->bindings[bindSkt]->bindedSkeletons[bindSkt2]),0);
                            (m->bindings[bindSkt]->bindedSkeletons[bindSkt2])->minSegmentLength = minValue;
                        }
                    }

                    fin = clock();
                    time_init = timelapse(fin,ini)*1000;
                    ini = clock();

                    // Cálculo de A
                    ComputeEmbeddingWithBD(*m);

                    fin = clock();
                    time_AComputation = timelapse(fin,ini)*1000;

                    vector< vector< vector<double> > >statistics;
                    statistics.resize(interiorPoints[i].size());
                    vector<float> time_MVC(interiorPoints[i].size(), -1);

                    QFile distFile(file+"_distances.txt");
                    if(!distFile.open(QFile::WriteOnly)) return;
                    QTextStream outDistances(&distFile);

                    for(int j = 0; j< interiorPoints[i].size(); j++)
                    {
                        // Calculo de distancias
                        vector<double> pointDistances;
                        vector<double> weights;
                        weights.resize(m->vn());
                        pointDistances.resize(m->vn());
                        statistics[j].resize(thresholds.size());

                        ini = clock();

                        mvcAllBindings(interiorPoints[i][j], weights, *m);

                        fin = clock();
                        time_MVC[j] = timelapse(fin,ini)*1000;

                        outDistances << "Point " << j << endl;

                        for(int k = 0; k < thresholds.size(); k++)
                        {
                            vector<int> weightsSort(m->vn());
                            vector<bool> weightsRepresentative(m->vn());

                            double currentTh = thresholds[k];

                            ini = clock();
                            doubleArrangeElements_withStatistics(weights, weightsSort, statistics[j][k], currentTh);

                            // 8 values
                            //statistics[0] = weights.size();
                            //statistics[1] = minValue;
                            //statistics[2] = maxValue;
                            //statistics[3] = sumValues;
                            //statistics[4] = countOutValues;
                            //statistics[5] = sumOutValues;
                            //statistics[6] = countNegValues;
                            //statistics[7] = sumNegValues;
                            

                            fin = clock();
                            time_MVC_sorting = timelapse(fin,ini)*1000;
                            statistics[j][k].push_back(time_MVC_sorting);
                            ini = clock();

                            double preValue = 0;
                            for(int currentBinding = 0; currentBinding < m->bindings.size(); currentBinding++)
                            {
                                int iniIdx = m->bindings[0]->globalIndirection.front();
                                int finIdx = m->bindings[0]->globalIndirection.back();
                                preValue += PrecomputeDistancesSingular_sorted(weights, weightsSort,  m->bindings[currentBinding]->BihDistances, currentTh);
                            }

                            fin = clock();
                            time_precomputedDistance = timelapse(fin,ini)*1000;
                            statistics[j][k].push_back(time_precomputedDistance);
                            ini = clock();

                            outDistances << "Threshold " << currentTh << endl ;

                            double maxDistance = -1;
                            for(int i = 0; i< m->modelVertexBind.size(); i++)
                            {
                                int iBind = m->modelVertexBind[i];
                                int iniIdx = m->bindings[iBind]->globalIndirection.front();
                                int finIdx = m->bindings[iBind]->globalIndirection.back();

                                int iVertexBind = m->modelVertexDataPoint[i];
                                pointDistances[i] = -BiharmonicDistanceP2P_sorted(weights, weightsSort, iVertexBind , m->bindings[iBind], 1.0, preValue, currentTh);

                                maxDistance = max(pointDistances[i], maxDistance);

                                outDistances << pointDistances[i] << ", ";
                            }

                            outDistances << endl;

                            fin = clock();
                            time_finalDistance = timelapse(fin,ini)*1000;
                            statistics[j][k].push_back(time_finalDistance);
                            statistics[j][k].push_back(maxDistance);
                        }
                        outDistances << endl;
                    }
                    distFile.close();
                    // Guardamos datos
                    //vector<vector<vector<float> > >statistics(interiorPoints[i].size());
                    //vector<float> > time_MVC(interiorPoints[i].size(), -1);
                    out << file+"_results.txt" << endl;
                    out << file+"_distances.txt" << endl;
                    out << file+"_pesos.txt" << endl;

                    QFile resFile(file+"_results.txt");
                    if(!resFile.open(QFile::WriteOnly)) return;
                    QTextStream outRes(&resFile);
                    outRes << "Init: " << time_init << " and AComputation: " << time_AComputation << " ms." << endl;
                    outRes << time_MVC[i] << endl;
                    for(int st01 = 0; st01 < time_MVC.size(); st01++)
                    {
                        outRes << QString("%1 ").arg(time_MVC[st01], 8);
                    }
                    outRes << " ms." << endl;
                    for(int st01 = 0; st01 < statistics.size(); st01++)
                    {
                        for(int st02 = 0; st02 < statistics[st01].size(); st02++)
                        {
                            for(int st03 = 0; st03 < statistics[st01][st02].size(); st03++)
                            {
                                outRes << QString("%1, ").arg(statistics[st01][st02][st03], 8);
                            }
                            outRes << endl;
                        }
                    }
                    resFile.close();

                    // Exportamos la segmentacion y los pesos en un fichero.
                    QFile pesosFile(file+"_pesos.txt");
                    if(!pesosFile.open(QFile::WriteOnly)) return;
                    QTextStream outPesos(&pesosFile);

                    FILE* foutLog = fopen((string(DIR_MODELS_FOR_TEST) + "outLog.txt").c_str(), "a");
                    fprintf(foutLog, "%s\n", modelFiles[i][mf].toStdString().c_str()); fflush(0);
                    fclose(foutLog);


                    // Calculo de pesos
                    for(int k = 0; k < thresholds.size(); k++)
                    {
                        for(int i = 0; i< m->bindings.size(); i++)
                        {
                            m->bindings[i]->weightsCutThreshold = thresholds[k];
                        }

                        printf("Computing skinning:\n");

                        // Realizamos el calculo para cada binding
                        clock_t iniImportante = clock();
                        ComputeSkining(*m);
                        clock_t finImportante = clock();

                        double timeComputingSkinning = timelapse(finImportante,iniImportante);

                        outPesos << "Threshold: " << thresholds[k] << " tiempo-> "<< timeComputingSkinning << endl;
                        char* charIntro;
                        scanf(charIntro);

                        for(int bindId = 0; bindId < m->bindings.size(); bindId++)
                        {
                            for(int pointBind = 0; pointBind < m->bindings[bindId]->pointData.size(); pointBind++)
                            {
                                PointData& pt = m->bindings[bindId]->pointData[pointBind];
                                outPesos << pt.ownerLabel << ", ";
                            }

                            outPesos << endl;
                        }
                    }

                    pesosFile.close();

                    // eliminamos el modelo
                    delete m;
                    escena->models.clear();

                }
            }
            outFile.close();

			*/
}

void GLWidget::BuildTetrahedralization()
{
	/*
	tetgenio in, out;
	tetgenio::facet *f;
	tetgenio::polygon *p;
	int i;

	
	// All indices start from 1.
	in.firstnumber = 1;

	in.numberofpoints = 8;
	in.pointlist = new REAL[in.numberofpoints * 3];
	in.pointlist[0]  = 0;  // node 1.
	in.pointlist[1]  = 0;
	in.pointlist[2]  = 0;
	in.pointlist[3]  = 2;  // node 2.
	in.pointlist[4]  = 0;
	in.pointlist[5]  = 0;
	in.pointlist[6]  = 2;  // node 3.
	in.pointlist[7]  = 2;
	in.pointlist[8]  = 0;
	in.pointlist[9]  = 0;  // node 4.
	in.pointlist[10] = 2;
	in.pointlist[11] = 0;
	// Set node 5, 6, 7, 8.
	for (i = 4; i < 8; i++) {
		in.pointlist[i * 3]     = in.pointlist[(i - 4) * 3];
		in.pointlist[i * 3 + 1] = in.pointlist[(i - 4) * 3 + 1];
		in.pointlist[i * 3 + 2] = 12;
	}

	in.numberoffacets = 6;
	in.facetlist = new tetgenio::facet[in.numberoffacets];
	in.facetmarkerlist = new int[in.numberoffacets];

	// Facet 1. The leftmost facet.
	f = &in.facetlist[0];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 1;
	p->vertexlist[1] = 2;
	p->vertexlist[2] = 3;
	p->vertexlist[3] = 4;
  
	// Facet 2. The rightmost facet.
	f = &in.facetlist[1];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 5;
	p->vertexlist[1] = 6;
	p->vertexlist[2] = 7;
	p->vertexlist[3] = 8;

	// Facet 3. The bottom facet.
	f = &in.facetlist[2];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 1;
	p->vertexlist[1] = 5;
	p->vertexlist[2] = 6;
	p->vertexlist[3] = 2;

	// Facet 4. The back facet.
	f = &in.facetlist[3];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 2;
	p->vertexlist[1] = 6;
	p->vertexlist[2] = 7;
	p->vertexlist[3] = 3;

	// Facet 5. The top facet.
	f = &in.facetlist[4];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 3;
	p->vertexlist[1] = 7;
	p->vertexlist[2] = 8;
	p->vertexlist[3] = 4;

	// Facet 6. The front facet.
	f = &in.facetlist[5];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 4;
	p->vertexlist[1] = 8;
	p->vertexlist[2] = 5;
	p->vertexlist[3] = 1;

	// Set 'in.facetmarkerlist'
	in.facetmarkerlist[0] = -1;
	in.facetmarkerlist[1] = -2;
	in.facetmarkerlist[2] = 0;
	in.facetmarkerlist[3] = 0;
	in.facetmarkerlist[4] = 0;
	in.facetmarkerlist[5] = 0;
	

	// Output the PLC to files 'barin.node' and 'barin.poly'.
	in.save_nodes("barin");
	in.save_poly("barin");

	// Tetrahedralize the PLC. Switches are chosen to read a PLC (p),
	//   do quality mesh generation (q) with a specified quality bound
	//   (1.414), and apply a maximum volume constraint (a0.1).

	tetrahedralize("pq1.414a0.1", &in, &out);

	

	escena->models.push_back((object*)new Modelo(escena->getNewId()));
	Modelo* m = ((Modelo*)escena->models.back());

	delete m->shading;
	m->shading = new GraphRender(m);

	ReBuildScene();

	m->nodes.resize(8);
	for(int points = 0; points< 8; points++)
	{
		m->nodes[points] = new GraphNode(points);
		m->nodes[points]->position = Vector3d( in.pointlist[points*3],in.pointlist[points*3+1],in.pointlist[points*3+2]);
	}

	m->nodes[0]->connections.push_back(m->nodes[1]);
	m->nodes[1]->connections.push_back(m->nodes[2]);
	m->nodes[2]->connections.push_back(m->nodes[3]);
	m->nodes[3]->connections.push_back(m->nodes[0]);

	m->nodes[1]->connections.push_back(m->nodes[0]);
	m->nodes[2]->connections.push_back(m->nodes[1]);
	m->nodes[3]->connections.push_back(m->nodes[2]);
	m->nodes[0]->connections.push_back(m->nodes[3]);

	//p->vertexlist[0] = 1;
	//p->vertexlist[1] = 2;
	//p->vertexlist[2] = 3;
	//p->vertexlist[3] = 4;

	m->nodes[4]->connections.push_back(m->nodes[5]);
	m->nodes[5]->connections.push_back(m->nodes[6]);
	m->nodes[6]->connections.push_back(m->nodes[7]);
	m->nodes[7]->connections.push_back(m->nodes[4]);

	m->nodes[5]->connections.push_back(m->nodes[4]);
	m->nodes[6]->connections.push_back(m->nodes[5]);
	m->nodes[7]->connections.push_back(m->nodes[6]);
	m->nodes[4]->connections.push_back(m->nodes[7]);

	//p->vertexlist[0] = 5;
	//p->vertexlist[1] = 6;
	//p->vertexlist[2] = 7;
	//p->vertexlist[3] = 8;
	
	m->nodes[0]->connections.push_back(m->nodes[4]);
	m->nodes[1]->connections.push_back(m->nodes[5]);
	m->nodes[2]->connections.push_back(m->nodes[6]);
	m->nodes[3]->connections.push_back(m->nodes[7]);

	m->nodes[4]->connections.push_back(m->nodes[0]);
	m->nodes[5]->connections.push_back(m->nodes[1]);
	m->nodes[6]->connections.push_back(m->nodes[2]);
	m->nodes[7]->connections.push_back(m->nodes[3]);
	*/

	/*p->vertexlist[0] = 1;
	p->vertexlist[1] = 5;
	p->vertexlist[2] = 6;
	p->vertexlist[3] = 2;
	
	p->vertexlist[0] = 2;
	p->vertexlist[1] = 6;
	p->vertexlist[2] = 7;
	p->vertexlist[3] = 3;
	
	p->vertexlist[0] = 3;
	p->vertexlist[1] = 7;
	p->vertexlist[2] = 8;
	p->vertexlist[3] = 4;
	
	p->vertexlist[0] = 4;
	p->vertexlist[1] = 8;
	p->vertexlist[2] = 5;
	p->vertexlist[3] = 1;*/
	
	/*for(int faces = 0; faces< 6; faces++)
	{
		m->nodes[points] = new GraphNode(points);
		m->nodes[points]->position = Vector3d( in.pointlist[points*3],in.pointlist[points*3+1],in.pointlist[points*3+2]);
	}*/

	emit updateSceneView();
}

void testGridValuesInterpolation()
{
	// test de interpolacion
	// Definimos un grid de 50x50x50
	/*
	Modelo* m2 = (Modelo*)escena->models[0];
	escena->visualizers.push_back((shadingNode*)new gridRenderer());
	gridRenderer* grRend2 = (gridRenderer*)escena->visualizers.back();
	grRend2->iam = GRIDRENDERER_NODE;
	grRend2->model = m2;
	Box3d newBound(Vector3d(0,0,0), Vector3d(10,10,10));
	Vector3i res(50, 50, 50);
	grRend2->grid = new grid3d(newBound, res, m2->nodes.size());
	
	for(int i = 0; i< res.X(); i++)
    {
        for(int j = 0; j< res.Y(); j++)
        {
            for(int k = 0; k< res.Z(); k++)
            {
                grRend2->grid->cells[i][j][k]->setType(INTERIOR);
				grRend2->grid->cells[i][j][k]->data = new cellData();
            }
        }
    }

	Vector3i  ptminIdx(0,0,0);
	Vector3i  ptmaxIdx(0,0,0);

	grRend2->grid->cells[0][0][0]->data->influences.push_back(weight(0,1));
	grRend2->grid->cells[0][0][0]->data->influences.push_back(weight(1,0));
	grRend2->grid->cells[0][0][0]->data->influences.push_back(weight(2,0));

	grRend2->grid->cells[0][49][0]->data->influences.push_back(weight(0,0));
	grRend2->grid->cells[0][49][0]->data->influences.push_back(weight(1,1));
	grRend2->grid->cells[0][49][0]->data->influences.push_back(weight(2,0));

	grRend2->grid->cells[0][49][49]->data->influences.push_back(weight(0,1));
	grRend2->grid->cells[0][49][49]->data->influences.push_back(weight(1,0));
	grRend2->grid->cells[0][49][49]->data->influences.push_back(weight(2,0));

	grRend2->grid->cells[0][0][49]->data->influences.push_back(weight(0,0));
	grRend2->grid->cells[0][0][49]->data->influences.push_back(weight(1,0));
	grRend2->grid->cells[0][0][49]->data->influences.push_back(weight(2,1));

	grRend2->grid->cells[49][0][0]->data->influences.push_back(weight(0,1));
	grRend2->grid->cells[49][0][0]->data->influences.push_back(weight(1,1));
	grRend2->grid->cells[49][0][0]->data->influences.push_back(weight(2,1));

	grRend2->grid->cells[49][49][0]->data->influences.push_back(weight(0,1));
	grRend2->grid->cells[49][49][0]->data->influences.push_back(weight(1,0));
	grRend2->grid->cells[49][49][0]->data->influences.push_back(weight(2,1));

	grRend2->grid->cells[49][49][49]->data->influences.push_back(weight(0,0));
	grRend2->grid->cells[49][49][49]->data->influences.push_back(weight(1,1));
	grRend2->grid->cells[49][49][49]->data->influences.push_back(weight(2,1));

	grRend2->grid->cells[49][0][49]->data->influences.push_back(weight(0,0));
	grRend2->grid->cells[49][0][49]->data->influences.push_back(weight(1,0));
	grRend2->grid->cells[49][0][49]->data->influences.push_back(weight(2,1));
	*/

	//interpolacion lineal
	/*for(int i = 1; i< 49; i++)
	{
		float interpolationValues = (float)i/49.0;
		interpolateLinear(grRend2->grid->cells[0][i][0]->data->influences, grRend2->grid->cells[0][0][0]->data->influences , grRend2->grid->cells[0][49][0]->data->influences, interpolationValues);
	}
	*/
	
	// interpolacion bilineal
	/*for(int i = 1; i< 49; i++)
	{
		float interpolationValues1 = (float)i/49.0;
		for(int j = 1; j< 49; j++)
		{
				float interpolationValues2 = (float)j/49.0;
				interpolateBiLinear(grRend2->grid->cells[0][i][j]->data->influences, 
								grRend2->grid->cells[0][0][0]->data->influences ,
								grRend2->grid->cells[0][49][0]->data->influences ,
								grRend2->grid->cells[0][49][49]->data->influences , 
								grRend2->grid->cells[0][0][49]->data->influences ,
								interpolationValues1, 
								interpolationValues2);
		}
	}
	*/

	/*
	vector< vector<weight>* > pts;

	pts.push_back(&grRend2->grid->cells[0][0][0]->data->influences);
	pts.push_back(&grRend2->grid->cells[0][49][0]->data->influences);
	pts.push_back(&grRend2->grid->cells[0][49][49]->data->influences);
	pts.push_back(&grRend2->grid->cells[0][0][49]->data->influences);

	pts.push_back(&grRend2->grid->cells[49][0][0]->data->influences);
	pts.push_back(&grRend2->grid->cells[49][49][0]->data->influences);
	pts.push_back(&grRend2->grid->cells[49][49][49]->data->influences);
	pts.push_back(&grRend2->grid->cells[49][0][49]->data->influences);

	for(int k = 1; k< 49; k++)
	{
		float interpolationValues3 = (float)k/49.0;
		for(int i = 1; i< 49; i++)
		{
			float interpolationValues1 = (float)i/49.0;
			for(int j = 1; j< 49; j++)
			{
					float interpolationValues2 = (float)j/49.0;
					interpolateTriLinear(grRend2->grid->cells[k][i][j]->data->influences, pts, interpolationValues1, interpolationValues2, interpolationValues3);
			}
		}
	}

	grRend2->updateGridColorsAndValuesRGB();

	return;
	*/
}

void GLWidget::paintModelWithData() {

	AirRig* rig = (AirRig*)escena->rig;
	if(!rig) return;

    for(unsigned int modelIdx = 0; modelIdx < escena->models.size(); modelIdx++)
    {
        Modelo* m = (Modelo*)escena->models[modelIdx];

		if(m->shading->colors.size() != m->vn())
            m->shading->colors.resize(m->vn());

        m->cleanSpotVertexes();

        double maxdistance = 0.001;
        vector<double> pointdistances;
        vector<double> maxdistances;
		maxdistances.resize(m->bind->surfaces.size());

        vector<double> weights;
        vector<int> weightssort(m->vn());
        vector<bool> weightsrepresentative(m->vn());

        vector<double> completedistances;
        double threshold = escena->weightsThreshold;//1/pow(10.0, 3);

		if(escena->iVisMode == VIS_ISODISTANCES || escena->iVisMode == VIS_POINTDISTANCES || escena->iVisMode == VIS_ERROR || escena->iVisMode == VIS_WEIGHT_INFLUENCE)
        {
			/*
            printf("tiempos de computacion para %d vertices: \n", m->vn()); fflush(0);

            clock_t ini, fin;
            ini = clock();

            weights.resize(m->vn());
            int currentbinding = 1;
            pointdistances.resize(m->vn());
            //pointdistances.resize(m->bindings[currentbinding]->pointdata.size());
            //mvcsinglebinding(interiorpoint, weights, m->bindings[currentbinding], *m);
            mvcAllBindings(interiorPoint, weights, m->bindings, *m);

            fin = clock();
            printf("mean value coordinates: %f ms\n", timelapse(fin,ini)*1000); fflush(0);
            ini = clock();

            //for(int si = 0; si < weights.size(); si++)
            //{
            //	weightssort[si] = si;
            //}
            //doublearrangeelements(weights, weightssort, true, threshold);
            vector<double> stats;
            doubleArrangeElements_withStatistics(weights, weightssort, stats, threshold);


            fin = clock();
            printf("ordenacion: %f ms\n", timelapse(fin,ini)*1000); fflush(0);
            ini = clock();

            double prevalue = 0;
            for(int currentbinding = 0; currentbinding < m->bindings.size(); currentbinding++)
            {
                int iniidx = m->bindings[currentbinding]->globalIndirection.front();
                int finidx = m->bindings[currentbinding]->globalIndirection.back();
                prevalue += PrecomputeDistancesSingular_sorted(weights, weightssort,  m->bindings[currentbinding]->BihDistances, threshold);
            }

            fin = clock();
            printf("precomputo: %f ms\n", timelapse(fin,ini)*1000); fflush(0);
            ini = clock();

            for(int i = 0; i< m->modelVertexBind.size(); i++)
            {
                int ibind = m->modelVertexBind[i];
                int iniidx = m->bindings[ibind]->globalIndirection.front();
                int finidx = m->bindings[ibind]->globalIndirection.back();

                int ivertexbind = m->modelVertexDataPoint[i];

                pointdistances[i] = -BiharmonicDistanceP2P_sorted(weights, weightssort, ivertexbind , m->bindings[ibind], 1.0, prevalue, threshold);

                maxdistances[ibind] = max(maxdistances[ibind],pointdistances[i]);
                maxdistance = max(pointdistances[i], maxdistance);
            }

            fin = clock();
            printf("calculo distancias: %f ms\n", timelapse(fin,ini)*1000); fflush(0);
            ini = clock();

            for(int md = 0; md< maxdistances.size(); md++)
            {
                printf("bind %d con maxdist: %f\n", md, maxdistances[md]);
                fflush(0);
            }
			*/

            if(escena->iVisMode == VIS_ERROR)
            {
				/*
                completedistances.resize(m->vn());
                double prevalue2 = 0;
                for(int currentbinding = 0; currentbinding < m->bindings.size(); currentbinding++)
                {
                    int iniidx = m->bindings[currentbinding]->globalIndirection.front();
                    int finidx = m->bindings[currentbinding]->globalIndirection.back();
                    prevalue2 += PrecomputeDistancesSingular_sorted(weights, weightssort,  m->bindings[currentbinding]->BihDistances, -10);
                }

                for(int i = 0; i< m->modelVertexBind.size(); i++)
                {
                    int ibind = m->modelVertexBind[i];
                    int iniidx = m->bindings[ibind]->globalIndirection.front();
                    int finidx = m->bindings[ibind]->globalIndirection.back();

                    int ivertexbind = m->modelVertexDataPoint[i];
                    completedistances[i] = -BiharmonicDistanceP2P_sorted(weights, weightssort, ivertexbind , m->bindings[ibind], 1.0, prevalue2, -10);
                }
				*/
            }
        }

        double maxError = -9999;
        if(!m->bind) continue;
        for(int count = 0; count< m->bind->pointData.size(); count++)
        {
            if(m->bind->pointData[count].isBorder)
                m->addSpotVertex(m->bind->pointData[count].node->id);

            float value = 0.0;
            // deberia ser al reves, recorrer el binding y este pinta los puntos del modelo.
            // hacer un reset antes con el color propio del modelo.
            binding* bd = m->bind;
            PointData& pd = bd->pointData[count];
            int newvalue = 0;
            if(escena->iVisMode == VIS_LABELS)
            {
				value = (float)pd.component/(float)m->bind->surfaces.size();
            }
            else if(escena->iVisMode == VIS_SEGMENTATION)
            {
                newvalue = (pd.segmentId-100)*13;
                value = ((float)(newvalue%50))/50.0;
            }
            else if(escena->iVisMode == VIS_BONES_SEGMENTATION)
            {
				newvalue = (rig->defRig.defNodesRef[pd.segmentId]->boneId-100)*13;
                //newvalue = (newvalue * 25) % 100;
                value = ((float)(newvalue%25))/25.0;
            }
            else if(escena->iVisMode == VIS_WEIGHTS)
            {
				int actualDesiredNumber = escena->desiredVertex;
				for(int groupIdx = 0; groupIdx < rig->defRig.defGroups.size(); groupIdx++)
				{
					if(rig->defRig.defGroups[groupIdx]->transformation->nodeId == actualDesiredNumber)
					{
						actualDesiredNumber = rig->defRig.defGroups[groupIdx]->nodeId;
						break;
					}
				}

                //float sum = 0;
                value = 0.0;

                int searchedindex = -1;
                for(unsigned int ce = 0; ce < pd.influences.size(); ce++)
                {
                    if(pd.influences[ce].label == actualDesiredNumber)
                    {
                        searchedindex = ce;
                        break;
                    }
                    //sum += pd.influences[ce].weightvalue;
                }
                if(searchedindex >= 0)
                        value = pd.influences[searchedindex].weightValue;

                //if (sum < 1)
                //	printf("no se cumple la particion de unidad: %f.\n", sum);
            }
            else if(escena->iVisMode == VIS_SEG_PASS)
            {
                value = 0.0;
				int actualDesiredNumber = escena->desiredVertex;
				for(int groupIdx = 0; groupIdx < rig->defRig.defGroups.size(); groupIdx++)
				{
					if(rig->defRig.defGroups[groupIdx]->transformation->nodeId == actualDesiredNumber)
					{
						actualDesiredNumber = rig->defRig.defGroups[groupIdx]->nodeId;
						break;
					}
				}

					
				if(rig->defRig.defNodesRef[pd.segmentId]->boneId == escena->desiredVertex)
                {
                    value = 1.0;
                }
            }
            else if(escena->iVisMode == VIS_CONFIDENCE_LEVEL)
            {
				value = 0.0;

				int actualDesiredNumber = escena->desiredVertex;
				for(int groupIdx = 0; groupIdx < rig->defRig.defGroups.size(); groupIdx++)
				{
					if(rig->defRig.defGroups[groupIdx]->transformation->nodeId == actualDesiredNumber)
					{
						actualDesiredNumber = rig->defRig.defGroups[groupIdx]->nodeId;
						break;
					}
				}

                int searchedindex = -1;
                for(unsigned int ce = 0; ce < pd.influences.size(); ce++)
                {
                    if(pd.influences[ce].label == actualDesiredNumber)
                    {
                        searchedindex = ce;
                        break;
                    }
                    //sum += pd.influences[ce].weightvalue;
                }
                if(searchedindex >= 0)
				{
					if(pd.secondInfluences[searchedindex].size()> 0)
					{
						if(valueAux < pd.secondInfluences[searchedindex].size())
							value = pd.secondInfluences[searchedindex][valueAux];
						else
							value = pd.secondInfluences[searchedindex][pd.secondInfluences[searchedindex].size()-1];
					}
					else
					{
						value = 1.0;
					}
				}

				/*
                value = 0.0;
                if(count == escena->desiredVertex)
                {
                    value = 1.0;
                }
                else
                {
                    for(int elemvecino = 0; elemvecino < bd->surface.nodes[count]->connections.size() ; elemvecino++)
                    {
                        if(bd->surface.nodes[count]->connections[elemvecino]->id == escena->desiredVertex)
                        {
                            value = 0.5;
                            break;
                        }
                    }
                }
				*/
            }
            else if(escena->iVisMode == VIS_ISODISTANCES)
            {
                if(escena->desiredVertex <0 || escena->desiredVertex >= bd->pointData.size())
                    value = 0;
                else
                {
                    if(maxdistance <= 0)
                        value = 0;
                    else
                        value = m->bind->BihDistances.get(count,escena->desiredVertex) / maxdistance;
                }
            }
            else if(escena->iVisMode == VIS_POINTDISTANCES)
            {
                if(maxdistance <= 0)
                    value = 0;
                else
                {
                    int modelvert = m->bind->pointData[count].node->id;
                    value = pointdistances[modelvert] / maxdistances[0];
                }
            }
            else if(escena->iVisMode == VIS_ERROR)
            {
				value = 0.0;

                int searchedindex = -1;
                for(unsigned int ce = 0; ce < pd.influences.size(); ce++)
                {
                    if(pd.influences[ce].label == escena->desiredVertex)
                    {
                        searchedindex = ce;
                        break;
                    }
                    //sum += pd.influences[ce].weightvalue;
                }
                if(searchedindex >= 0)
				{
					if(pd.secondInfluences[searchedindex].size()> 0)
					{
						if(valueAux < pd.secondInfluences[searchedindex].size())
							value = pd.secondInfluences[searchedindex][valueAux]*pd.influences[searchedindex].weightValue;
						else
							value = pd.secondInfluences[searchedindex][pd.secondInfluences[searchedindex].size()-1]*pd.influences[searchedindex].weightValue;
					}
					else
					{
						value = 1.0*pd.influences[searchedindex].weightValue;
					}
				}
                /*int modelvert = m->bindings[currentbinding]->pointData[count].node->id;

                if(completedistances[modelvert] > 0)
                    value = pointdistances[modelvert] - completedistances[modelvert];

                maxError = max((double)maxError, (double)value);
				*/

            }
            else if(escena->iVisMode == VIS_WEIGHT_INFLUENCE)
            {


                int modelVert = m->bind->pointData[count].node->id;
                value = weights[modelVert];
                //if(maxDistance <= 0)
                //	value = 0;
                //else
                //{
                //	int modelVert = m->bindings[currentBinding]->pointData[count].modelVert;
                //	value = pointDistances[modelVert] / maxDistances[currentBinding];
                //}
            }
            else
            {
                value = 1.0;
            }

            float r,g,b;
            GetColourGlobal(value,0.0,1.0, r, g, b);
            //QColor c(r,g,b);
            m->shading->colors[bd->mainSurface->nodes[count]->id].resize(3);
            m->shading->colors[bd->mainSurface->nodes[count]->id][0] = r;
            m->shading->colors[bd->mainSurface->nodes[count]->id][1] = g;
            m->shading->colors[bd->mainSurface->nodes[count]->id][2] = b;
        }
    }
}

void GLWidget::selectElements(vector<unsigned int > lst)
{
	// model and skeleton
	AdriViewer::selectElements(lst);

	if( !escena || !escena->rig) return;

	AirRig* rig = (AirRig*)escena->rig;
	// rig
	for(unsigned int i = 0; i< lst.size(); i++)
    {
		for(int group = 0; group < rig->defRig.defGroups.size(); group++)
		{
			if(rig->defRig.defGroups[group]->nodeId == lst[i])
			{
				// pos, rot, expansion, smoothing, twist
				joint* jt = rig->defRig.defGroups[group]->transformation;
				emit jointDataShow(rig->defRig.defGroups[group]->expansion, rig->defRig.defGroups[group]->nodeId);

				emit defGroupData(rig->defRig.defGroups[group]->iniTwist, 
								  rig->defRig.defGroups[group]->finTwist, 
								  rig->defRig.defGroups[group]->enableTwist,
								  rig->defRig.defGroups[group]->smoothingPasses);

				double alfa,beta,gamma;
				toEulerAngles(jt->qrot, alfa, beta, gamma);
				emit jointTransformationValues(jt->pos[0], jt->pos[1], jt->pos[2], alfa, beta, gamma);

				selMgr.selection.push_back((object*)rig->defRig.defGroups[group]);
			}
		}
	}
}

void GLWidget::computeProcess() {

	//Testing new optimized processing
	// AirRig creation
	if(escena->rig)
		delete escena->rig;

	// Crear rig nuevo
	escena->rig = new AirRig(scene::getNewId());
	AirRig* rig = (AirRig*) escena->rig;

	// Get values from UI
	float subdivisionRatio = parent->ui->bonesSubdivisionRatio->text().toFloat();

	//Vincular a escena: modelo y esqueletos
	rig->bindRigToModelandSkeleton((Modelo*)escena->models[0], escena->skeletons, subdivisionRatio);

	// Build deformers structure for computations.
	rig->preprocessModelForComputations();

	// Skinning computations
	updateAirSkinning(rig->defRig, *rig->model);

	// enable deformations and render data
	rig->enableDeformation = true;
	paintModelWithData();

	emit updateSceneView();

	/*

	// Get all the parameters.
	float subditionRatio = parent->ui->bonesSubdivisionRatio->text().toFloat();

	// Test de interpolacion
	//testGridValuesInterpolation();

	// Voxelize the model and compute HC.
	if(!useMVC)
	{
		printf("Computing HC grid.\n"); fflush(0);

		// Just taking only the first model
		Modelo* m = (Modelo*)escena->models[0];
		
		// Gird visualizer creation
		escena->visualizers.push_back((shadingNode*)new gridRenderer());
		gridRenderer* grRend = (gridRenderer*)escena->visualizers.back();
		grRend->iam = GRIDRENDERER_NODE;
		grRend->model = m;

		// grid creation for computation
		grRend->grid = new grid3d();
		QString sGridFileName = (QString("%1%2_gridHC.bin").arg(m->sPath.c_str()).arg(m->sName.c_str()));
		if(!QFile(sGridFileName).exists())
			getHC_insideModel(*m, *grRend->grid, pow(2,6), sGridFileName.toStdString().c_str());
		else
			grRend->grid->LoadGridFromFile(sGridFileName.toStdString().c_str());
	

		m->HCgrid = grRend->grid;
	}
    
	if(escena->models.size() <= 0) return;

    printf("Preparing in data:\n"); fflush(0);
	clock_t iniProcess = clock();
    for(unsigned int i = 0; i< escena->models.size(); i++)
    {
        Modelo* m = (Modelo*)escena->models[i];
        printf("Modelo: %s\n", m->sModelPrefix.c_str());

		// Process to fusion several parts.
        bool usePatches = false;
        if(usePatches)
        {
			printf("Computing virtual triangles.\n");
            AddVirtualTriangles(*m);
        }
		else
		{
			printf("We consider the model as a single piece\n");
		}

        // Si no se ha calculado las distancias biharmonicas lo hacemos
        if(!m->computedBindings)
        {
            char bindingFileName[150];
            char bindingFileNameComplete[150];
            sprintf(bindingFileName, "%s/bind_%s", m->sPath.c_str(), m->sName.c_str());
            sprintf(bindingFileNameComplete, "%s.bin", bindingFileName);

            bool ascii = false;

            // A. Intentamos cargarlo
            ifstream myfile;
            myfile.open (bindingFileNameComplete, ios::in |ios::binary);
            bool loaded = false;
            if (myfile.is_open())
            {
                // En el caso de existir simplemente tenemos que cargar las distancias.
                loaded = LoadEmbeddings(*m, bindingFileNameComplete);
            }

            //B. En el caso de que no se haya cargado o haya incongruencias con lo real, lo recomputamos.
            if(!loaded)
            {
                bool success = ComputeEmbeddingWithBD(*m);
                if(!success)
                {
                    printf("[ERROR - computeProcess] No se ha conseguido computar el embedding\n");
                    fflush(0);
                    return;
                }
                else SaveEmbeddings(*m, bindingFileName, ascii);
            }
        }

		// That was a preprocess for the distances that could be recovered, 
		// somehow there will be a reason for stabilize the data.
        //normalizeDistances(*m);

		printf("Linking skeletons with models:\n");
        // de momento asignamos todos los esqueletos a todos los bindings... ya veremos luego.
        for(int i = 0; i< escena->skeletons.size(); i++)
        {
            float minValue = GetMinSegmentLenght(getMinSkeletonSize((skeleton*)escena->skeletons[i]),0);
            ((skeleton*)escena->skeletons[i])->minSegmentLength = subditionRatio * minValue;

            for(int bind = 0; bind< m->bindings.size(); bind++)
                m->bindings[bind]->bindedSkeletons.push_back((skeleton*)escena->skeletons[i]);
        }

        for(int i = 0; i< m->bindings.size(); i++)
        {
            m->bindings[i]->weightsCutThreshold = 1;//escena->weightsThreshold;
        }

		clock_t finProcess = clock();
        printf("Until here: %f s.\n Now computing skinning:\n", double(timelapse(iniProcess,finProcess)));
		fflush(0);

        // Realizamos el calculo para cada binding
		clock_t ini = clock();
        ComputeSkining(*m);
		clock_t fin = clock();

        //reportResults(*m, m->bindings[bind]);

        printf("Computed skinning has taken %f s.\n", double(timelapse(ini,fin)));
        fflush(0);

		// save the binding computed
		saveBinding( m->bindings[0], m->sPath+"/"+m->sName+"_binding.txt");
    }

    paintModelWithData();
    //paintPlaneWithData();

	*/
}

void GLWidget::paintPlaneWithData(bool compute)
{
	/*
    Modelo& m = *((Modelo*)escena->models[0]);

    clipingPlaneRenderer* planeRender = NULL;
    for(int i = 0; i< escena->visualizers.size(); i++)
    {
        if(escena->visualizers[i]->iam == PLANERENDERER_NODE)
        {
            planeRender = (clipingPlaneRenderer*)escena->visualizers[i];
            break;
        }
    }

    m.setSpotVertex(escena->desiredVertex);

    int subdivisions = 50;
    if(!planeRender)
    {
        escena->visualizers.push_back(new clipingPlaneRenderer(escena->getNewId()));
        planeRender = (clipingPlaneRenderer*)escena->visualizers.back();

        Vector3d AxisX(1,0,0);
        Vector3d AxisY(0,1,0);
        Vector3d AxisZ(0,0,1);

        double length = (m.maxBBox - m.minBBox).Norm();
        Vector3d center = (m.maxBBox + m.minBBox)/2.0;
        planeRender->minPoint = Vector3d(center[0]-length/2,center[1]-length/2,center[2]-length/2);
        planeRender->maxPoint = Vector3d(center[0]+length/2,center[1]+length/2,center[2]+length/2);
        planeRender->plane->addTranslation(center[0]-length/2,center[1]-length/2,center[2]-length/2);
        planeRender->subdivisions = subdivisions;
        planeRender->points.resize(subdivisions*subdivisions);

        vector<Vector3d>& points = planeRender->points;
        double pass = length/subdivisions;
        for(unsigned int planoX = 0; planoX < subdivisions; planoX++)
        {
            for(unsigned int planoZ = 0; planoZ < subdivisions; planoZ++)
            {
                points[planoX*subdivisions+planoZ] = Vector3d(planoX*pass, planoZ*pass, 0);
            }
        }
    }
    vector<Vector3f>& colors = planeRender->colors;
    colors.resize(planeRender->points.size());

    vector< vector<double> >& weights = planeRender->weights;
    if(compute || planeRender->dirtyFlag)
    {
        weights.resize(planeRender->points.size());

        int bindingIdx = 0;
        for(int i = 0; i < planeRender->points.size(); i++)
        {
            //mvcSingleBinding(planeRender->points[i]+planeRender->plane->pos, weights[i], m.bindings[0], m);
            mvcAllBindings(planeRender->points[i] + planeRender->plane->pos, weights[i], m);
        }
    }

    // Ordenamos los pesos de cada punto del plano
    vector< vector<int> >weightsSort(weights.size());
    for(int i = 0; i < weights.size(); i++) weightsSort[i].resize(m.vn());
    vector< vector<bool> > weightsRepresentative(weights.size());
    for(int i = 0; i < weights.size(); i++) weightsRepresentative[i].resize(m.vn());
    double threshold = -10;//1/pow(10.0, 3);

    for(int i = 0; i < weights.size(); i++)
        doubleArrangeElements(weights[i], weightsSort[i], false, threshold);

    if(planeRender->vmode == VIS_WEIGHTS_ONPLANE)
    {
        double minValue = 1;
        double maxValue = 0;
        for(unsigned int planoX = 0; planoX < planeRender->subdivisions; planoX++)
        {
            for(unsigned int planoZ = 0; planoZ < planeRender->subdivisions; planoZ++)
            {
                maxValue = max(weights[planoX*planeRender->subdivisions+planoZ][m.globalIndirection[escena->desiredVertex]], maxValue);
                minValue = min(weights[planoX*planeRender->subdivisions+planoZ][m.globalIndirection[escena->desiredVertex]], minValue);
            }
        }

        printf("Valor de peso min: %f\n",minValue); fflush(0);
        printf("Valor de peso max: %f\n",maxValue); fflush(0);

        for(unsigned int planoX = 0; planoX < planeRender->subdivisions; planoX++)
        {
            for(unsigned int planoZ = 0; planoZ < planeRender->subdivisions; planoZ++)
            {
                float r,g,b;
                double weightAux = weights[planoX*planeRender->subdivisions+planoZ][m.globalIndirection[escena->desiredVertex]];

                //GetColour(weightAux, minValue, maxValue, r, g, b);
                GetColourGlobal(weightAux, minValue, maxValue, r, g, b);
                colors[planoX*planeRender->subdivisions+planoZ] = Vector3f(r,g,b);
            }
        }

        planeRender->posUpdated = true;
    }
    else if(planeRender->vmode == VIS_DISTANCES)
    {
        vector<double> pointDistances;
        pointDistances.resize(planeRender->points.size());
        double maxDistance = 0;
        for(int planePoint = 0; planePoint < planeRender->points.size(); planePoint++)
        {
            double preValue = 0;
            preValue = PrecomputeDistancesSingular_sorted(weights[planePoint], weightsSort[planePoint], m.bindings[0]->BihDistances, threshold);

            // solo calculamos hacia un vertice de la maya
            pointDistances[planePoint] = -BiharmonicDistanceP2P_sorted(weights[planePoint], weightsSort[planePoint], escena->desiredVertex , m.bindings[0], 1.0, preValue, threshold);
            maxDistance = max(pointDistances[planePoint], maxDistance);
        }

        for(unsigned int planoX = 0; planoX < planeRender->subdivisions; planoX++)
        {
            for(unsigned int planoZ = 0; planoZ < planeRender->subdivisions; planoZ++)
            {
                float r,g,b;
                double distance = pointDistances[planoX*planeRender->subdivisions+planoZ];

                //GetColour(weightAux, minValue, maxValue, r, g, b);
                GetColourGlobal(distance, 0, maxDistance, r, g, b);
                colors[planoX*planeRender->subdivisions+planoZ] = Vector3f(r,g,b);
            }
        }
    }
	*/
}

void GLWidget::VoxelizeModel(Modelo* m, bool onlyBorders)
{
    /*
    // Hemos vinculado el grid al modelo y esqueleto cargados.
    // Hay que gestionar esto con cuidado.
    printf("Ojo!, usamos los modelos y skeletos tal cual...\n");
    escena->visualizers.push_back((shadingNode*)new gridRenderer());
    gridRenderer* grRend = (gridRenderer*)escena->visualizers.back();
    grRend->iam = GRIDRENDERER_NODE;
    grRend->model = m;

    // grid creation for computation
    m->grid = new grid3d();
    grRend->grid = m->grid;

    grRend->grid->res = parent->ui->gridResolutionIn->text().toInt();
    grRend->grid->worldScale = parent->ui->sceneScale->text().toInt();

    for(int i = 0; i< escena->skeletons.size(); i++)
    {
        float cellSize = grRend->grid->cellSize;
        GetMinSegmentLenght(getMinSkeletonSize((skeleton*)escena->skeletons[i]), cellSize);
        ((skeleton*)escena->skeletons[i])->minSegmentLength = GetMinSegmentLenght(getMinSkeletonSize((skeleton*)escena->skeletons[i]), cellSize);

        grRend->grid->bindedSkeletons.push_back((skeleton*)escena->skeletons[i]);
    }

    clock_t begin, end;

    vector< vector<double> >* embedding = NULL;
    if(m->embedding.size() != 0)
    {
        embedding = &(m->embedding);
    }
    else // No se ha cargado ningun embedding.
    {
        printf("No hay cargado ning�n embedding. Lo siento, no puedo hacer calculos\n"); fflush(0);
        return;
    }

    // Cargamos el grid si ya ha sido calculado
    QString sSavingFile = QString("%1%2%3").arg(m->sPath.c_str()).arg(m->sName.c_str()).arg("_embedded_grid.dat");
    if(!sSavingFile.isEmpty() && QFile(sSavingFile).exists())
    {
        if(VERBOSE)
        {
           cout << "Loading from disc: ";
           begin=clock();
        }

        grRend->grid->LoadGridFromFile(sSavingFile.toStdString());

        if(VERBOSE)
        {
            end = clock();
            cout << double(timelapse(end,begin)) << " s"<< endl;
        }
    }
    else
    {
        if(VERBOSE)
        {
            if(!onlyBorders) cout << "Computing volume: " << endl;
            else cout << "Computing volume (onlyAtBorders): " << endl;

            cout << "------------------------------------------------------" << endl;
            cout << "                  - PREPROCESO -                      " << endl;
            cout << "------------------------------------------------------" << endl;
            cout << "Creaci�n del grid con res " << grRend->grid->res << endl;
            begin=clock();

        }

        // CONSTRUCCION DEL GRID
        gridInit(*m,*(grRend->grid));

        if(VERBOSE)
        {
            end=clock();
            cout << double(timelapse(end,begin)) << " s"<< endl;
            begin = end;
            cout << "Etiquetado del grid: ";
        }

        //Tipificamos las celdas seg�n si es interior, exterior, o borde de la maya
        grRend->grid->typeCells(*m);

        if(VERBOSE)
        {
            end=clock();
            cout << double(timelapse(end,begin)) << " s"<< endl;
            cout << "Grid cells embedding";
            begin=clock();
        }

        // EMBEDING INTERPOLATION FOR EVERY CELL
        int interiorCells = 0;
        interiorCells = gridCellsEmbeddingInterpolation(*m, *(grRend->grid), m->embedding, onlyBorders);

        if(VERBOSE)
        {
            end=clock();
            cout << "("<< interiorCells <<"cells): " << double(timelapse(end,begin)) << " s"<< endl;
            double memorySize = (double)(interiorCells*DOUBLESIZE*embedding[0].size())/MBSIZEINBYTES;
            cout << "Estimated memory consumming: " << memorySize << "MB" <<endl;
            cout << ">> TOTAL (construccion del grid desde cero): "<<double(timelapse(end,begin)) << " s"<< endl;

        }

        if(!sSavingFile.isEmpty())
        {
            if(VERBOSE)
            {
               cout << "Guardando en disco: ";
               begin=clock();
            }

            grRend->grid->SaveGridToFile(sSavingFile.toStdString());

            if(VERBOSE)
            {
                clock_t end=clock();
                cout << double(timelapse(end,begin)) << " s"<< endl;
            }
        }
    }

    grRend->propagateDirtyness();
    updateGridRender();
    */
}

// Do test skinning
/*
void GLWidget::doTestsSkinning(string fileName, string name, string path)
{

    // Realizaremos los tests especificados en un fichero.
    QFile file(QString(fileName.c_str()));

    if(!file.open(QFile::ReadOnly)) return;

    QTextStream in(&file);

    QString resultFileName = in.readLine();
    QString workingPath = in.readLine();

    QStringList globalCounts = in.readLine().split(" ");
    int modelCount = globalCounts[0].toInt();
    int interiorPointsCount = globalCounts[1].toInt();

    vector<QString> modelPaths(modelCount);
    vector<QString> modelSkeletonFile(modelCount);
    vector<QString> modelNames(modelCount);
    vector< vector<QString> >modelFiles(modelCount);
    vector< vector<Vector3d> >interiorPoints(modelCount);

    QStringList thresholdsString = in.readLine().split(" ");
    vector< double >thresholds(thresholdsString.size());
    for(int i = 0; i< thresholds.size(); i++)
        thresholds[i] = thresholdsString[i].toDouble();

    for(int i = 0; i< modelCount; i++)
    {
        modelNames[i] = in.readLine();
        modelPaths[i] = in.readLine();
        modelSkeletonFile[i] = in.readLine();
        modelFiles[i].resize(in.readLine().toInt());

        for(int j = 0; j < modelFiles[i].size(); j++)
            modelFiles[i][j] = in.readLine();

        QStringList intPointsModel = in.readLine().split(" ");
        interiorPoints[i].resize(interiorPointsCount);

        assert(intPointsModel.size() == interiorPointsCount*3);

        for(int j = 0; j< intPointsModel.size(); j = j+3)
        {
            double v1 = intPointsModel[j].toDouble();
            double v2 = intPointsModel[j+1].toDouble();
            double v3 = intPointsModel[j+2].toDouble();
            interiorPoints[i][j/3] = Vector3d(v1, v2, v3);
        }
    }
    file.close();

    // Preparamos y lanzamos el calculo.
    QFile outFile(workingPath+"/"+resultFileName);
    if(!outFile.open(QFile::WriteOnly)) return;
    QTextStream out(&outFile);

    // time variables.
    float time_init = -1;
    float time_AComputation = -1;
    float time_MVC_sorting = -1;
    float time_precomputedDistance = -1;
    float time_finalDistance = -1;

    for(int i = 0; i< modelCount; i++)
    {
        for(int mf = 0; mf< modelFiles[i].size(); mf++)
        {
            //Leemos el modelo
            QString file = workingPath+"/"+modelPaths[i]+"/"+modelFiles[i][mf];
            QString path = workingPath+"/"+modelPaths[i]+"/";
            readModel( file.toStdString(), modelNames[i].toStdString(), path.toStdString());
            Modelo* m = ((Modelo*)escena->models.back());

            QString sktFile = workingPath+"/"+modelPaths[i]+"/"+modelSkeletonFile[i];

            clock_t ini, fin;
            ini = clock();

            // Incializamos los datos
            BuildSurfaceGraphs(*m, m->bindings);

            for(int bindSkt = 0; bindSkt < m->bindings.size(); bindSkt++)
                readSkeletons(sktFile.toStdString(), m->bindings[bindSkt]->bindedSkeletons);

            for(int bindSkt = 0; bindSkt < m->bindings.size(); bindSkt++)
            {
                for(int bindSkt2 = 0; bindSkt2 < m->bindings[bindSkt]->bindedSkeletons.size(); bindSkt2++)
                {
                    float minValue = GetMinSegmentLenght(getMinSkeletonSize(m->bindings[bindSkt]->bindedSkeletons[bindSkt2]),0);
                    (m->bindings[bindSkt]->bindedSkeletons[bindSkt2])->minSegmentLength = minValue;
                }
            }

            fin = clock();
            time_init = timelapse(fin,ini)*1000;
            ini = clock();

            // Cálculo de A
            ComputeEmbeddingWithBD(*m);

            fin = clock();
            time_AComputation = timelapse(fin,ini)*1000;

            vector< vector< vector<double> > >statistics;
            statistics.resize(interiorPoints[i].size());
            vector<float> time_MVC(interiorPoints[i].size(), -1);

            // Guardamos datos
            //vector<vector<vector<float> > >statistics(interiorPoints[i].size());
            //vector<float> > time_MVC(interiorPoints[i].size(), -1);
            out << file+"_results.txt" << endl;
            out << file+"_distances.txt" << endl;
            out << file+"_pesos.txt" << endl;

            // Exportamos la segmentacion y los pesos en un fichero.
            QFile pesosFile(file+"_pesos.txt");
            if(!pesosFile.open(QFile::WriteOnly)) return;
            QTextStream outPesos(&pesosFile);

            // Calculo de pesos
            for(int k = 0; k < thresholds.size(); k++)
            {
                for(int i = 0; i< m->bindings.size(); i++)
                {
                    m->bindings[i]->weightsCutThreshold = thresholds[k];
                }

                printf("Computing skinning:\n");

                // Realizamos el calculo para cada binding
                clock_t iniImportante = clock();
                ComputeSkining(*m);
                clock_t finImportante = clock();

                double timeComputingSkinning = timelapse(finImportante,iniImportante);

                outPesos << "Threshold: " << thresholds[k] << " tiempo-> "<< timeComputingSkinning << endl;
                char* charIntro;
                scanf(charIntro);

                for(int bindId = 0; bindId < m->bindings.size(); bindId++)
                {
                    for(int pointBind = 0; pointBind < m->bindings[bindId]->pointData.size(); pointBind++)
                    {
                        PointData& pt = m->bindings[bindId]->pointData[pointBind];
                        outPesos << pt.ownerLabel << ", ";
                    }

                    outPesos << endl;
                }
            }

            pesosFile.close();

            // eliminamos el modelo
            delete m;
            escena->models.clear();

        }
    }
    outFile.close();

}
*/

// Lee una o varias cajas del disco
/*
// Lee una o varias cajas del disco
// filename: fichero de ruta completa.
// model: MyMesh donde se almacenara la/s cajas leida/s.
void AdriViewer::readCage(QString fileName, Modelo& m_)
{
   // Desglosamos el fichero en 3 partes:
   //    1. Path donde est� el modelo
   //    2. Prefijo del fichero
   //    3. Extensi�n del fichero.
   if(!loadedModel) return; // Es necesario cargar el modelo antes.

   QString ext = fileName.right(3);
   QFileInfo sPathAux(fileName);
   //m_.sModelPath = aux.toStdString();

   bool loaded = false;
   if(ext == "off")
   {
       //fileName cage:
       m_.cage.Clear();
       m_.dynCage.Clear();

        tri::io::ImporterOFF<MyMesh>::Open(m_.cage,fileName.toStdString().c_str());
       gpUpdateNormals(m_.cage, false);
        tri::Append<MyMesh, MyMesh>::Mesh(m_.dynCage,m_.cage, false);
       gpUpdateNormals(m_.dynCage, false);

       loadedModel = loaded = true;
       updateInfo();

       viewingMode = DynCage_Mode;
   }
   else if(ext == "txt")
   {
       //fileName cage:
       m_.cage.Clear();
       m_.dynCage.Clear();

       QFile stillCagesDef(fileName);
       if(stillCagesDef.exists())
       {
           stillCagesDef.open(QFile::ReadOnly);
           QTextStream sciTS(&stillCagesDef);
           int stillCagesCount = 0;
           sciTS >> stillCagesCount;
           if(stillCagesCount > 0)
               m_.stillCages.resize(stillCagesCount);

           QString sPathModels = QString(m_.sModelPath.c_str())+"/";

           QString str;
           QString cageName;

           sciTS >> cageName >> str;
           printf("Caja original: %s\n", (sPathModels+str).toStdString().c_str()); fflush(0);
            tri::io::ImporterOFF<MyMesh>::Open(m_.cage,cageName.toStdString().c_str());
           gpUpdateNormals(m_.cage, false);

           for(unsigned int i = 0; i< m_.stillCages.size(); i++)
           {
               str = ""; cageName = "";

               sciTS >> cageName >> str;
               printf("Caja %d: %s\n", i, (sPathModels+str).toStdString().c_str()); fflush(0);

               if(!QFile((sPathModels+str)).exists())
                   continue;

               parent->ui->cagesComboBox->addItem(cageName, i);

               m_.stillCages[i] = new MyMesh();
                tri::io::ImporterOFF<MyMesh>::Open(*m_.stillCages[i],(sPathModels+str).toStdString().c_str());
               gpUpdateNormals(*m_.stillCages[i], false);
           }
           stillCagesDef.close();

           viewingMode = Cages_Mode;
       }

       loadedCages = loaded = true;
       updateInfo();

   }
   else
       return;


   if(loaded)
   {
        tri::UpdateBounding<MyMesh>::Box(m_.modeloOriginal);
        tri::UpdateBounding<MyMesh>::Box(m_.cage);

       //buildCube(cage, model.bbox.Center(), model.bbox.Diag());
       //loadSelectVertexCombo(cage);
       loadSelectableVertex(m_.dynCage);

       // Podriamos poner un switch para leer varios tipos de modelos.

       ReBuildScene();
   }

}
*/

void GLWidget::exportWeightsToMaya()
{
    assert(false);
    // esta vinculado al grid

    /*
   gridRenderer* grRend = (gridRenderer*)escena->visualizers.back();
   Modelo* m = (Modelo*)escena->models.back();
   skeleton* skt = (skeleton*)escena->skeletons.back();

   vector< vector<float> > meshWeights;

   meshWeights.resize(m->vn());

   int idxCounter = 0;
   MyMesh::VertexIterator vi;
   for(vi = m->vert.begin(); vi!=m->vert.end(); ++vi )
   {
       Vector3i pt = grRend->grid->cellId((*vi).P());
       cell3d* cell = grRend->grid->cells[pt.X()][pt.Y()][pt.Z()];

       meshWeights[idxCounter].resize(grRend->grid->valueRange, 0.0);

       for(unsigned int w = 0; w< cell->data->influences.size(); w++)
       {
           meshWeights[idxCounter][cell->data->influences[w].label] =  cell->data->influences[w].weightValue;
       }

       idxCounter++;
   }

   vector<string> names;
   skt->GetJointNames(names);

   string fileName = QString("%1%2.weights").arg(m->sPath.c_str()).arg(m->sName.c_str()).toStdString();
   saveWeightsToMayaByName(m, meshWeights, names, fileName);
   */
}

void GLWidget::UpdateVertexSource(int id)
{
    escena->desiredVertex = id;
    paintModelWithData();   // TODO paint model
    //paintPlaneWithData();

    updateGridRender();

    //TODO
    /*
   double vmin = 9999999, vmax = -9999999;
   for(int i = 0; i < m.modeloOriginal.vn; i++)
   {
       vmin = min(BHD_distancias[i][id], vmin);
       vmax = max(BHD_distancias[i][id], vmax);
   }

   printf("Distancias-> max:%f min:%f\n", vmax, vmin); fflush(0);

   vertexColors.resize(m.modeloOriginal.vn);
   for(int i = 0; i < m.modeloOriginal.vn; i++)
   {
       vertexColors[i] = GetColour(BHD_distancias[i][id], vmin, vmax);
   }

   colorsLoaded = true;

   updateGL();
   */
}

void GLWidget::importSegmentation(QString fileName)
{
    //TODO

    /*
    // Leemos contenido del paquete
    QFile file(fileName);
    file.open(QFile::ReadOnly);
    QTextStream in(&file);

    QString indicesFileName;
    indicesFileName = in.readLine();

    int maxIdx = -1;

    QString dummy;
    int number_of_points;
    in >> number_of_points;

    //printf("%d\n", number_of_points); fflush(0);

    if(number_of_points <= 0)
        return;

    BHD_indices.resize(m.modeloOriginal.vn);
    int counter = 0;
    for(int i = 0; i< m.modeloOriginal.vn && !in.atEnd(); i++)
    {
        in >> dummy;
        //printf("%s ", dummy.toStdString().c_str()); fflush(0);

        for(int j = 0; j< number_of_points && !in.atEnd(); j++)
        {
            int tempData;
            in >> tempData;

            //printf("%d ", tempData); fflush(0);
            if(j == 0)
            {
                BHD_indices[i] = tempData; // Cogemos el primero
            }

            maxIdx = max(maxIdx, BHD_indices[i]);
            counter++;
        }
        //printf("\n"); fflush(0);
    }

    if(counter != m.modeloOriginal.vn)
        printf("No coincide el numero de vertices con colores.\n");

    if(maxIdx <= 0) return;

    printf("Distancias-> %d elementos\n", maxIdx); fflush(0);

    updateColorLayersWithSegmentation(maxIdx);
    */
}

void GLWidget::updateColorLayersWithSegmentation(int maxIdx)
{
    //TODO
    assert(false);
    /*
    vertexColorLayers.resize(2);
    int segLayer = 0;
    int spotLayer = 1;

    vertexColorLayers[segLayer].resize(m.modeloOriginal.vn);
    vertexColorLayers[spotLayer].resize(m.modeloOriginal.vn);

    MyMesh::VertexIterator vii;
    for(vii = m.modeloOriginal.vert.begin(); vii!=m.modeloOriginal.vert.end(); ++vii )
    {
        //vector< int > idxCounters(maxIdx+1, 0);

        int different = 0;
        int neighbours = 0;
        int idxOfVii = BHD_indices[vii->IMark()];
         face::VFIterator<MyFace> vfi(&(*vii)); //initialize the iterator to the first face
        for(;!vfi.End();++vfi)
        {
          MyFace* f = vfi.F();

          for(int i = 0; i<3; i++)
          {
            MyVertex * v = f->V(i);
            if (v->IMark() == vii->IMark())continue;

            if( BHD_indices[v->IMark()] != idxOfVii )
                different++;

            neighbours++;
          }
       }

        if(different > neighbours/2.0)
            vertexColorLayers[spotLayer][vii->IMark()] = QColor(255,0,0);
        else
            vertexColorLayers[spotLayer][vii->IMark()] = QColor(255,255,255);

        vertexColorLayers[segLayer][vii->IMark()] = GetColour(BHD_indices[vii->IMark()], 0, maxIdx);
    }

    colorLayers = true;
    colorsLoaded = true;
    viewingMode = BHD_Mode;

    updateGL()
    */
}

bool GLWidget::processGreenCoordinates()
{
    //TODO
    /*
    QTime time;
    time.start();

    //newModeloGC.Clear();
    // tri::Append<MyMesh, MyMesh>::Mesh(newModeloGC,modeloOriginal, false);
    //gpCalculateGreenCoordinates( modeloOriginal, cage, PerVertGC, PerFaceGC );

    m.processGreenCoordinates();

    // Para luego pintar bien todo.
    selectionValuesForColorGC.resize(m.modeloOriginal.vn, 0);

    double timeLapse = time.elapsed()/1000.0;
    QString texto("Ha tardado %1 segundos en obtener las coordenadas de green.");
    parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto.arg(timeLapse)));
    bGCcomputed = true;

    deformMesh();
    updateGL();
    */
    return true;
}

bool GLWidget::processMeanValueCoordinates()
{

    //TODO
    /*
    QTime time;
    time.start();

    //newModeloGC.Clear();
    // tri::Append<MyMesh, MyMesh>::Mesh(newModeloGC,modeloOriginal, false);
    //gpCalculateGreenCoordinates( modeloOriginal, cage, PerVertGC, PerFaceGC );

    m.processMeanValueCoordinates();

    // Para luego pintar bien todo.
    selectionValuesForColorMVC.resize(m.modeloOriginal.vn, 0);

    double timeLapse = time.elapsed()/1000.0;
    QString texto("Ha tardado %1 segundos en obtener las coordenadas de mean value.");
    parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto.arg(timeLapse)));
    bMVCComputed = true;

    deformMesh();
    updateGL();
    */
    return true;
}

bool GLWidget::processHarmonicCoordinates()
{
	/*
    //TODO
    
    QTime time;
    time.start();

    //newModeloHC.Clear();
    // tri::Append<MyMesh, MyMesh>::Mesh(newModeloHC,modeloOriginal, false);

    //unsigned int resolution = pow(2,7);
    //QString sHCSavedGrid = QDir::currentPath()+"/"+HC_GRID_FILE_NAME;

    //if(QFile(sHCSavedGrid).exists())
    //    loadHarmonicCoordinates(modeloOriginal, HCgrid, PerVertHC, sHCSavedGrid.toStdString());
    //else
//        getHarmonicCoordinates(modeloOriginal, cage, HCgrid, PerVertHC, resolution, sHCSavedGrid.toStdString());

    m.processHarmonicCoordinates();

    parent->ui->SliceSelector->setMinimum(0);
    parent->ui->SliceSelector->setMaximum(m.HCgrid.dimensions.Y()-1);

    selectionValuesForColorHC.resize(m.modeloOriginal.vn, 0);

    double timeLapse = time.elapsed()/1000.0;
    QString texto("Ha tardado %1 segundos en obtener las coordenadas harmonicas.");
    parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto.arg(timeLapse)));
    bHComputed = true;
    deformMesh();
    updateGL();
    */
    return true;
}

void GLWidget::active_GC_vs_HC(int tab)
{
    //TODO
    /*
    if(tab == 0) // GC
    {
        activeCoords = GREEN_COORDS;
    }
    else if(tab == 1)
    {
        activeCoords = HARMONIC_COORDS;
    }
    else if(tab == 2)
    {
        activeCoords = MEANVALUE_COORDS;
    }
    */
}

bool GLWidget::processAllCoords()
{
    //TODO
    /*
    QTime time;
    QTime total;
    time.start();
    total.start();

    //newModeloGC.Clear();
    // tri::Append<MyMesh, MyMesh>::Mesh(newModeloGC,modeloOriginal, false);
    //gpCalculateGreenCoordinates( modeloOriginal, cage, PerVertGC, PerFaceGC );

    QString texto("Obteniendo Green Coords....");
    parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto));
    m.processGreenCoordinates();
    selectionValuesForColorGC.resize(m.modeloOriginal.vn, 0);

    double timeLapse = time.elapsed()/1000.0;time.start();
    texto = QString("Green: en %1, Obteniendo Harmoni Coords....");
    parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto.arg(timeLapse)));
    m.processHarmonicCoordinates();
    parent->ui->SliceSelector->setMinimum(0);
    parent->ui->SliceSelector->setMaximum(m.HCgrid.dimensions.Y()-1);
    selectionValuesForColorHC.resize(m.modeloOriginal.vn, 0);

    timeLapse = time.elapsed()/1000.0;time.start();
    texto = QString("Harmonic: en %1, Obteniendo Mean Value Coords....");
    parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto.arg(timeLapse)));
    m.processMeanValueCoordinates();
    selectionValuesForColorMVC.resize(m.modeloOriginal.vn, 0);

    // Para luego pintar bien todo.
    timeLapse = time.elapsed()/1000.0;
    double totalLapse = total.elapsed()/1000.0;
    texto = QString("Mean value: en %1, total en %2....");
    parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto.arg(timeLapse)).arg(texto.arg(totalLapse)));

    bGCcomputed = true;
    bHComputed = true;
    bMVCComputed = true;

    deformMesh();
    updateGL();
    */
    return true;

}


bool GLWidget::deformMesh()
{

    //TODO
    /*
    if(activeCoords == HARMONIC_COORDS)
    {
        if(!bHComputed)
            return true;

        QTime time;
        time.start();

        if(stillCageAbled && stillCageSelected >= 0)
            m.deformMesh(stillCageSelected, HARMONIC_COORDS );
        else
            m.deformMesh(DYN_CAGE_ID, HARMONIC_COORDS );
        //deformMeshWithHC(modeloOriginal, cage, newModeloHC, newCage, PerVertHC);

        //parent->ui->deformedMeshCheck->setEnabled(true);
        QString texto("y %1 ms en procesar el nuevo modelo con HC.");
        parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto.arg(time.elapsed())));
        return true;
    }
    else if(activeCoords == GREEN_COORDS)
    {
        if(!bGCcomputed)
            return true;

        QTime time;
        time.start();

        if(stillCageAbled && stillCageSelected >= 0)
            m.deformMesh(stillCageSelected, GREEN_COORDS );
        else
            m.deformMesh(DYN_CAGE_ID, GREEN_COORDS );
        //deformMeshWithGC(modeloOriginal, cage, newModeloGC, newCage, PerVertGC, PerFaceGC);
        //parent->ui->deformedMeshCheck->setEnabled(true);
        QString texto("y %1 ms en procesar el nuevo modelo con GC.");
        parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto.arg(time.elapsed())));
        return true;
    }
    else if(activeCoords == MEANVALUE_COORDS)
    {
        if(!bMVCComputed)
            return true;

        QTime time;
        time.start();

        if(stillCageAbled && stillCageSelected >= 0)
            m.deformMesh(stillCageSelected, MEANVALUE_COORDS );
        else
            m.deformMesh(DYN_CAGE_ID, MEANVALUE_COORDS );
        //deformMeshWithGC(modeloOriginal, cage, newModeloGC, newCage, PerVertGC, PerFaceGC);
        //parent->ui->deformedMeshCheck->setEnabled(true);
        QString texto("y %1 ms en procesar el nuevo modelo con MVC.");
        parent->ui->infoBar->setText(QString(INFO_STRING).arg(texto.arg(time.elapsed())));
        return true;
    }

    */
    return false;
}

// TOFIX
void GLWidget::showDeformedModelSlot()
{
    //showDeformedModel = parent->ui->deformedMeshCheck->isChecked();
    updateGL();
}


void GLWidget::showHCoordinatesSlot()
{
    //m_bShowHCGrid = parent->ui->HCGridDraw->isChecked();
    //m_bShowHCGrid_interior = parent->ui->GridDraw_interior->isChecked();
    //m_bShowHCGrid_exterior = parent->ui->GridDraw_exterior->isChecked();
    //m_bShowHCGrid_boundary = parent->ui->GridDraw_boundary->isChecked();
    //m_bShowAllGrid = parent->ui->allGrid_button->isChecked();

    //m_bDrawHCInfluences = parent->ui->drawInfluences_check->isChecked();

    //if(!m_bShowAllGrid)
    //    parent->ui->SliceSelector->setEnabled(true);

    //ChangeSlice(parent->ui->SliceSelector->value());
    //updateGL();
}

// TOFIX
 


void GLWidget::updateGridVisualization()
{
    for(unsigned int i = 0; i< escena->visualizers.size(); i++)
    {
        if(!escena->visualizers[i] || escena->visualizers[i]->iam != GRIDRENDERER_NODE)
            continue;

        paintGrid((gridRenderer*)escena->visualizers[i]);
        paintModelWithGrid();
        ((gridRenderer*)escena->visualizers[i])->propagateDirtyness();
    }
}

void GLWidget::changeVisualizationMode(int mode)
{
    escena->iVisMode = mode;

    paintModelWithData();

    for(unsigned int i = 0; i< escena->visualizers.size(); i++)
    {
        if(!escena->visualizers[i] || escena->visualizers[i]->iam != GRIDRENDERER_NODE)
            continue;

         ((gridRenderer*)escena->visualizers[i])->iVisMode = mode;
         paintGrid((gridRenderer*)escena->visualizers[i]);
         paintModelWithGrid();
         ((gridRenderer*)escena->visualizers[i])->propagateDirtyness();
    }
}

void GLWidget::cleanWeights(gridRenderer* grRend)
{
    for(unsigned int i = 1; i < grRend->grid->cells.size()-1; i++)
    {
        for(unsigned int j = 1; j < grRend->grid->cells[i].size()-1; j++)
        {
            for(unsigned int k = 1; k < grRend->grid->cells[i][j].size()-1; k++)
            {
                if(grRend->grid->cells[i][j][k]->getType() != BOUNDARY) continue;

                cell3d* cell = grRend->grid->cells[i][j][k];
                cell->data->influences.clear();
            }
        }
    }
}

void GLWidget::setLocalSmoothPasses(int localSmooth)
{
	object *selectedObject = NULL;
    if (selMgr.selection.size() > 0)
        selectedObject = selMgr.selection.back();

	if (selectedObject != NULL) 
	{
		if(selectedObject->iam == DEFGROUP_NODE)
		{
			DefGroup* group = (DefGroup*) selectedObject;
			group->smoothingPasses = localSmooth;
			group->localSmooth = true;

			AirRig* rig = (AirRig*)escena->rig;
			updateAirSkinning(rig->defRig, *rig->model);
		}
	}

	paintModelWithData();
}

void GLWidget::setGlobalSmoothPasses(int value)
{	
	AirRig* rig = (AirRig*)escena->rig;
	if(rig)
	{
		rig->defaultSmoothPasses = value;
		for(int defIdx = 0; defIdx < rig->defRig.defGroups.size(); defIdx++)
		{
			if(!rig->defRig.defGroups[defIdx]->localSmooth)
				rig->defRig.defGroups[defIdx]->smoothingPasses = value;
		}

		updateAirSkinning(rig->defRig, *rig->model);
	}

	paintModelWithData();
}

void GLWidget::setTwistParams(double ini, double fin, bool enable)
{
	object *selectedObject = NULL;
    if (selMgr.selection.size() > 0)
        selectedObject = selMgr.selection.back();

	if (selectedObject != NULL) 
	{
		if(selectedObject->iam == DEFGROUP_NODE)
		{
			DefGroup* group = (DefGroup*) selectedObject;
			group->iniTwist = ini;
			group->finTwist = fin;
			group->enableTwist = enable;
		}
		/*
			AirRig* rig = (AirRig*)escena->rig;
			if(rig)
			{
				rig->iniTwist = ini;
				rig->finTwist = fin;
				rig->enableTwist = enable;
			}
		*/
	}

	paintModelWithData();
}


void GLWidget::changeSmoothPropagationDistanceRatio(float smoothRatioValue)
{
	// TODEBUG: estoy reescribiendo esta función para calcular con smoothing variable.

	/*
	for(int mod = 0; mod < escena->models.size(); mod++)
	{
			binding* bd = ((Modelo*)escena->models[mod])->bind;
			
			bd->smoothPropagationRatio = smoothRatioValue;

			assert(false);
			//TOFIX...
			//computeHierarchicalSkinning(*((Modelo*)escena->models[mod]), bd);
	}

	paintModelWithData();

	*/


	/*
    smoothRatioValue = (float)0.15;
    if(DEBUG) printf("changeSmoothPropagationDistanceRatio %f\n", smoothRatioValue); fflush(0);

    // TODEBUG
    // Estamos actuando sobre todos los grid renderer, deberia ser solo por el activo.
    for(unsigned int i = 0; i< escena->visualizers.size(); i++)
    {
        if(!escena->visualizers[i] || escena->visualizers[i]->iam != GRIDRENDERER_NODE)
            continue;

         ((gridRenderer*)escena->visualizers[i])->grid->smoothPropagationRatio = smoothRatioValue;
         ((gridRenderer*)escena->visualizers[i])->propagateDirtyness();
    }

    //TODEBUG
    printf("Ojo!, ahora estamos teniendo en cuenta: 1 solo modelo, esqueleto y grid.\n"); fflush(0);
    gridRenderer* grRend = (gridRenderer*)escena->visualizers.back();

    if(grRend == NULL) return;

    // limpiar los pesos calculados anteriormente
    cleanWeights(grRend);

    // recalcular los pesos
    assert(false);
    // He cambiado el grid por una malla: TODO y poner la siguiente instruccion
    //computeHierarchicalSkinning(*(grRend->grid));

    grRend->propagateDirtyness();
    updateGridRender();
	*/
}

void GLWidget::PropFunctionConf()
{
    // TODEBUG
    printf("Ojo!, ahora estamos teniendo en cuenta: 1 solo modelo, esqueleto y grid.\n"); fflush(0);
    Modelo* m = (Modelo*)escena->models.back();
    gridRenderer* grRend = (gridRenderer*)escena->visualizers.back();

    if(m == NULL || grRend == NULL) return;

    // He cambiaso el grid por la maya... hay quye hacer los cambios pertinentes.
    assert(false);
    // TODO

    /*
    // Cogemos los valores del usuario
    grRend->grid->kValue = parent->ui->kvalue_in->text().toFloat();
    grRend->grid->alphaValue = parent->ui->alphavalue_in->text().toFloat();
    grRend->grid->metricUsed = parent->ui->metricUsedCheck->isChecked();

    // limpiar los pesos calculados anteriormente
    cleanWeights(grRend);

    // recalcular los pesos
    computeHierarchicalSkinning(*(grRend->grid));

    grRend->propagateDirtyness();
    updateGridRender();
    */
}

void GLWidget::setPlaneData(bool drawPlane, int pointPos, int mode, float sliderPos, int orient)
{
    for(int i = 0; i< escena->visualizers.size(); i++)
    {
        if(escena->visualizers[i]->iam == PLANERENDERER_NODE)
        {
            clipingPlaneRenderer* planeRender;
            planeRender = (clipingPlaneRenderer*)escena->visualizers[i];
            planeRender->selectedPoint = pointPos;
            planeRender->vmode = (visualizatoinMode)mode;
            planeRender->visible = drawPlane;


            planeRender->position = sliderPos;
            planeRender->posUpdated = false;

            if(orient == 0) // X
            {
                double length = (planeRender->maxPoint[0]-planeRender->minPoint[0]);
                planeRender->plane->pos = planeRender->minPoint + Vector3d(length*sliderPos,0,0);
            }
            else if(orient == 1) // Y
            {
                double length = (planeRender->maxPoint[1]-planeRender->minPoint[1]);
                planeRender->plane->pos = planeRender->minPoint + Vector3d(0,length*sliderPos,0);
            }
            else if(orient == 2) // Z
            {
                double length = (planeRender->maxPoint[2]-planeRender->minPoint[2]);
                planeRender->plane->pos = planeRender->minPoint + Vector3d(0,0,length*sliderPos);
            }

            planeRender->dirtyFlag = true;
        }
    }
}


void GLWidget::setThreshold(double value)
{
    if(value == -10)
        escena->weightsThreshold = -10;
    else if(value == 0)
        escena->weightsThreshold = 0;
    else
        escena->weightsThreshold = 1/pow(10,value);
    paintModelWithData();
}

void GLWidget::allNextProcessSteps()
{
    /*
    for(int i = 0; i< CurrentProcessJoints.size(); i++)
    {
        computeHierarchicalSkinning(*(currentProcessGrid->grid), CurrentProcessJoints[i]);
    }
    CurrentProcessJoints.clear();
    */
}

void GLWidget::nextProcessStep()
{
    /*
    if(CurrentProcessJoints.size() > 0)
    {
        joint* jt = CurrentProcessJoints[0];
        for(int i = 1; i< CurrentProcessJoints.size(); i++)
        {
            CurrentProcessJoints[i-1] = CurrentProcessJoints[i];
        }

        CurrentProcessJoints.pop_back();

        computeHierarchicalSkinning(*(currentProcessGrid->grid), jt);
    }
    */
}

void loadDataForDrawing(gridRenderer* grRend )
{
    int borderCounter = 0;
    for(unsigned int i = 0; i< grRend->grid->cells.size(); i++)
    {
        for(unsigned int j = 0; j< grRend->grid->cells[i].size(); j++)
        {
            for(unsigned int k = 0; k< grRend->grid->cells[i][j].size(); k++)
            {
                cell3d* cell = grRend->grid->cells[i][j][k];
                if(cell->getType() == BOUNDARY)
                {
                    borderCounter++;
                }
            }
        }
    }

    grRend->grid->borderCellsCounts = borderCounter;
}

void reportResults(Modelo& model, binding* bb)
{
    assert(false);
     // esta vinculado al grid

    /*

    //printf("getting results %d points and %d nodes.\n",weights.points, weights.nodes);
    int count = 0;
    float sum = 0;
    MyMesh::VertexIterator vi;

//	grid3d& grid = *(model->grid);

    printf("Hola"); fflush(0);

    FILE* fout = fopen("secondaryWeights.txt", "w");
    for(vi = model.vert.begin(); vi!=model.vert.end(); ++vi )
    {

        // Obtener valores directamente de la estructura de datos del modelo.
        //TODO
        assert(false);

        // Obtener celda
        Vector3d ptPos = vi->P();
        Vector3i pt = grid.cellId(ptPos);
        cell3d* cell = grid.cells[pt.X()][pt.Y()][pt.Z()];

        fprintf(fout, "Influences: %d -> ", cell->data->influences.size()); fflush(fout);

        // copiar pesos
        for(unsigned int w = 0; w< cell->data->influences.size(); w++)
        {
            float auxWeight = cell->data->influences[w].weightValue;
            int idDeformer = cell->data->influences[w].label;
            //escena->defIds[idDeformer];
            fprintf(fout, "(%d,  %f) ", idDeformer, auxWeight); fflush(fout);
        }
        fprintf(fout, "\n"); fflush(fout);
        fprintf(fout, "Secondary Influences: %d -> ", cell->data->auxInfluences.size()); fflush(fout);
        for(unsigned int w = 0; w< cell->data->auxInfluences.size(); w++)
        {
            float auxWeight = cell->data->auxInfluences[w].weightValue;
            int idDeformer = cell->data->auxInfluences[w].label;
            int idRelDeformer = cell->data->auxInfluences[w].relativeLabel;

            //escena->defIds[idDeformer];
            fprintf(fout, "(%d,%d:%f) ", idDeformer,idRelDeformer, auxWeight); fflush(fout);
        }
        fprintf(fout, "\n\n"); fflush(fout);
        count++;

    }

    */
}

void GLWidget::getStatisticsFromData(string fileName, string name, string path)
{
    QFile file(QString(fileName.c_str()));
    if(!file.open(QFile::ReadOnly)) return;
    QTextStream in(&file);

    //Leemos los datos.
    vector<QString> paths;
    vector< vector<QString> > fileNames;

    int numPaths = in.readLine().toInt();
    paths.resize(numPaths);
    for(int i = 0; i < numPaths; i++)
        paths[i] = in.readLine();

    int numFiles = in.readLine().toInt();
    fileNames.resize(numFiles);
    for(int i = 0; i < numFiles; i++)
    {
        int numResolutions = in.readLine().toInt();
        fileNames[i].resize(numResolutions);
        for(int j = 0; j < numResolutions; j++)
        {
            fileNames[i][j] = in.readLine();
        }
    }

    file.close();


    for(int i = 0; i < fileNames.size(); i++)
    {
        for(int j = 0; j < fileNames[i].size(); j++)
        {
            vector< vector<int> >labels;
            labels.resize(paths.size());
            for(int k = 0; k < paths.size(); k++)
            {
                QFile file2(QString(paths[k] + fileNames[i][j]));
                if(!file2.open(QFile::ReadOnly))
                    continue;

                QTextStream inFile(&file2);

                QString dummy = inFile.readLine();

                int value;
                while(!inFile.atEnd())
                {
                    inFile >> value;
                    labels[k].push_back(value);
                    inFile >> dummy;
                }
                file2.close();
            }

            vector<int> counters;
            counters.resize(labels.size(),0);
            int countTotal;
            for(int l = 1; l< labels.size(); l++)
            {
                countTotal = labels[0].size();
                for(int c = 0; c< labels[0].size(); c++)
                {
                    if(labels[0][c] == labels[l][c]) counters[l]++;
                }
            }

            FILE* fout = fopen((string(DIR_MODELS_FOR_TEST) + "stats_aciertos.txt").c_str(), "a");
            fprintf(fout, "Results: %s\n", fileNames[i][j].toStdString().c_str());
            for(int l = 1; l < labels.size(); l++)
            {
                fprintf(fout, "Aciertos[%d]: %d de %d -> %f\n", l, counters[l], countTotal, (double)counters[l]/(double)countTotal);
            }
            fprintf(fout, "\n");
            fclose(fout);
        }
    }

    file.close();

}

void GLWidget::readDistances(QString fileName)
{
    // Leemos contenido del paquete
    int indSize = 0, vertSize = 0;
    QFile file(fileName);
    file.open(QFile::ReadOnly);
    QTextStream in(&file);
    QString indicesFileName, distancesFileName;
    indicesFileName = in.readLine();
    distancesFileName = in.readLine();
    in >> vertSize; // Leemos el numero de vert del modelo
    in >> indSize; // Leemos el numero de indices calculados.


    file.close();

    if(vertSize == 0 || indSize == 0) return;

    int i = 0;
    // Leemos indices
    if(indSize == vertSize)
    {
        BHD_indices.resize(indSize);
        for(unsigned int i = 0; i< BHD_indices.size(); i++)
            BHD_indices[i] = i;
    }
    else
    {
        QFile indFile(indicesFileName);
        indFile.open(QFile::ReadOnly);
        QTextStream inIndices(&indFile);
        BHD_indices.resize(indSize);

        int dummy = 0;
        while(!inIndices.atEnd())
        {
           inIndices >> dummy;
           BHD_indices[i] = dummy;
           i++;
        }
        indFile.close();
    }

    // Leemos las distancias
    QFile distFile(distancesFileName);
    distFile.open(QFile::ReadOnly);
    QTextStream inDistances(&distFile);

    /* LECTURA TOTAL */
    /*
    i = 0;
    int fila = 0, cola = 0;
    while(!inDistances.atEnd())
    {
       if(i%indSize == 0)
       {
         BHD_distancias[fila].resize(indSize);
         fila++;
       }

       inIndices >> dummy;
       BHD_distancias[fila][cola] = dummy;
       i++;
       cola++;
    }
    distFile.close();
    */

    BHD_distancias.resize(vertSize);
    for(int k = 0; k < indSize; k++)
    {
        BHD_distancias[k].resize(indSize);
        BHD_distancias[k][k] = 0;
    }

    /* LECTURA TIPO MATLAB */
    i = 0;
    QString dummy = "0";
    int fila = 0, cola = 0;
    while(!inDistances.atEnd())
    {
        i++;
        cola++;

        if(i%indSize == 0)
        {
          fila++;
          cola = fila+1;
          i += fila+1;
        }

        inDistances >> dummy;
        if(fila < indSize && cola < indSize)
        {
            BHD_distancias[fila][cola] = dummy.toDouble();
            BHD_distancias[cola][fila] = dummy.toDouble();
        }

    }
    distFile.close();

    viewingMode = BHD_Mode;
}

/*
void AdriViewer::drawCube(Vector3d o, double cellSize, Vector3f color, bool blend)
{
    Vector3f v[8];

    v[0] = Vector3f(o.X(), o.Y()+cellSize, o.Z());
    v[1] = Vector3f(o.X(), o.Y()+cellSize, o.Z()+cellSize);
    v[2] = Vector3f(o.X(), o.Y(), o.Z()+cellSize);
    v[3] = Vector3f(o.X(), o.Y(), o.Z());
    v[4] = Vector3f(o.X()+cellSize, o.Y()+cellSize, o.Z());
    v[5] = Vector3f(o.X()+cellSize, o.Y()+cellSize, o.Z()+cellSize);
    v[6] = Vector3f(o.X()+cellSize, o.Y(), o.Z()+cellSize);
    v[7] = Vector3f(o.X()+cellSize, o.Y(), o.Z());

    glDisable(GL_LIGHTING);

    if(blend)
        glColor4f(color[0], color[1], color[2], 0.3);
    else
        glColor3fv(&color[0]);

    glBegin(GL_QUADS);
    // queda reconstruir el cubo y ver si se pinta bien y se ha calculado correctamente.
       glNormal3f(-1,0,0);
       glVertex3fv(&v[0][0]); glVertex3fv(&v[1][0]); glVertex3fv(&v[2][0]); glVertex3fv(&v[3][0]);

       glNormal3f(1,0,0);
       glVertex3fv(&v[7][0]); glVertex3fv(&v[6][0]); glVertex3fv(&v[5][0]); glVertex3fv(&v[4][0]);

       glNormal3f(0,1,0);
       glVertex3fv(&v[1][0]); glVertex3fv(&v[5][0]); glVertex3fv(&v[6][0]); glVertex3fv(&v[2][0]);

       glNormal3f(0,-1,0);
       glVertex3fv(&v[0][0]); glVertex3fv(&v[3][0]); glVertex3fv(&v[7][0]); glVertex3fv(&v[4][0]);

       glNormal3f(0,0,1);
       glVertex3fv(&v[6][0]); glVertex3fv(&v[7][0]); glVertex3fv(&v[3][0]); glVertex3fv(&v[2][0]);

       glNormal3f(0,0,-1);
       glVertex3fv(&v[0][0]); glVertex3fv(&v[4][0]); glVertex3fv(&v[5][0]); glVertex3fv(&v[1][0]);
    glEnd();
    glEnable(GL_LIGHTING);
}
*/

// activamos el modo invisible.
void GLWidget::toogleToShowSegmentation(bool toogle)
{
    for(unsigned int i = 0; i< escena->visualizers.size(); i++)
    {
        if(!escena->visualizers[i] || escena->visualizers[i]->iam != GRIDRENDERER_NODE)
            continue;

         ((gridRenderer*)escena->visualizers[i])->bshowSegmentation = toogle;
         ((gridRenderer*)escena->visualizers[i])->propagateDirtyness();
    }
}

void GLWidget::changeExpansionFromSelectedJoint(float expValue)
{
	object *selectedObject = NULL;
	if (selMgr.selection.size() > 0)
		selectedObject = selMgr.selection.back();

	if (selectedObject != NULL) 
	{
		if(selectedObject->iam == DEFGROUP_NODE)
		{
			DefGroup* group = (DefGroup*) selectedObject;
			group->expansion = expValue;
			propagateExpansion(*group);

			AirRig* rig = (AirRig*)escena->rig;
			updateAirSkinning(rig->defRig, *rig->model);
		}
	}

	paintModelWithData();

	/*
     for(unsigned int i = 0; i< selMgr.selection.size(); i++)
     {
         if(((object*)selMgr.selection[i])->iam == JOINT_NODE)
         {
             joint* jt = (joint*)selMgr.selection[i];
             jt->expansion = expValue;

             for(unsigned int i = 0; i< escena->visualizers.size(); i++)
             {
                 if(!escena->visualizers[i] || escena->visualizers[i]->iam != GRIDRENDERER_NODE)
                     continue;

                  vector<DefNode>& nodes = ((gridRenderer*)escena->visualizers[i])->grid->v.intPoints;

                  for(unsigned int n = 0; n< nodes.size(); n++)
                  {
                      if(((DefNode)nodes[n]).boneId == (int)jt->nodeId)
                      {
                          if(((DefNode)nodes[n]).ratio < ratioExpansion_DEF)
                          {
                              float ratio2 = (((DefNode)nodes[n]).ratio/ratioExpansion_DEF);
                              float dif = 1-expValue;
                              float newValue =  expValue + dif*ratio2;
                              nodes[n].expansion = newValue;
                          }
                      }
                  }
             }

         }
     }

     //TODEBUG
     printf("Ojo!, ahora estamos teniendo en cuenta: 1 solo modelo, esqueleto y grid.\n"); fflush(0);
     gridRenderer* grRend = (gridRenderer*)escena->visualizers.back();

     if(grRend == NULL) return;

     //TODO: he cambiado el grid por la maya directamente, hay que rehacer la siguiente llamada.
     assert(false);
     //updateSkinningWithHierarchy(*(grRend->grid));

     grRend->propagateDirtyness();
     updateGridRender();
	 */


 }

void GLWidget::postSelection(const QPoint&)
{
    int i = 0;
    //TODO
    /*
    if(!bHComputed && !bGCcomputed && !bMVCComputed )
        return;

    maxMinGC = Point2f(-99999, 99999);
    maxMinHC = Point2f(-99999, 99999);
    maxMinMVC = Point2f(-99999, 99999);

    if(bGCcomputed)
        for(unsigned int i = 0; i< selectionValuesForColorGC.size(); i++)
            selectionValuesForColorGC[i] = 0;

    if(bHComputed)
       for(unsigned int i = 0; i< selectionValuesForColorHC.size(); i++)
            selectionValuesForColorHC[i] = 0;

    if(bMVCComputed)
       for(unsigned int i = 0; i< selectionValuesForColorMVC.size(); i++)
            selectionValuesForColorMVC[i] = 0;

    MyMesh::VertexIterator vi;
    for(vi = m.modeloOriginal.vert.begin(); vi!=m.modeloOriginal.vert.end(); ++vi )
    {
        float valueSum = 0;
        float valueSum2 = 0;
        float valueSum3 = 0;
        for (QList<int>::const_iterator it=selection_.begin(), end=selection_.end(); it != end; ++it)
        {
            if(bGCcomputed)
                valueSum += m.PerVertGC[vi->IMark()][*it];

            if(bHComputed)
                valueSum2 += m.PerVertHC[vi->IMark()][*it];

            if(bMVCComputed)
                valueSum3 += m.PerVertMVC[vi->IMark()][*it];
        }

        if(bGCcomputed)
        {
            selectionValuesForColorGC[vi->IMark()] = valueSum;
            maxMinGC[0] = max(maxMinGC[0], valueSum);
            maxMinGC[1] = min(maxMinGC[1], valueSum);
        }

        if(bHComputed)
        {
            selectionValuesForColorHC[vi->IMark()] = valueSum2;
            maxMinHC[0] = max(maxMinHC[0], valueSum2);
            maxMinHC[1] = min(maxMinHC[1], valueSum2);
        }

        if(bMVCComputed)
        {
            selectionValuesForColorMVC[vi->IMark()] = valueSum3;
            maxMinMVC[0] = max(maxMinMVC[0], valueSum3);
            maxMinMVC[1] = min(maxMinMVC[1], valueSum3);

        }
    }

    printf("\nMax GC: %f Min GC: %f \n", maxMinGC[0], maxMinGC[1]);
    printf("\nMax HC: %f Min HCC: %f \n", maxMinHC[0], maxMinHC[1]);
    printf("\nMax MVC: %f Min MVC: %f \n\n", maxMinMVC[0], maxMinMVC[1]);
    fflush(0);
    */
}

void GLWidget::drawWithDistances()
{
    //TODO
    /*
    if(ShadingModeFlag)
       glShadeModel(GL_SMOOTH);
    else
       glShadeModel(GL_FLAT);

    bool useColorLayers = (colorLayers && colorLayerIdx >= 0 && colorLayerIdx < vertexColorLayers.size());
    fflush(0);
    if(loadedModel)
    {
        // Pintamos en modo directo el modelo
        glBegin(GL_TRIANGLES);
        glColor3f(1.0, 1.0, 1.0);
        MyMesh::FaceIterator fi;
        int faceIdx = 0;
        for(fi = m.modeloOriginal.face.begin(); fi!=m.modeloOriginal.face.end(); ++fi )
         {
             //glNormal3dv(&(*fi).N()[0]);
             for(int i = 0; i<3; i++)
             {
                 int id = fi->V(i)->IMark();
                 if((colorsLoaded&&!colorLayers) || useColorLayers)
                 {
                     if(useColorLayers)
                     {
                        glColor3f(vertexColorLayers[colorLayerIdx][id].redF(),vertexColorLayers[colorLayerIdx][id].greenF(), vertexColorLayers[colorLayerIdx][id].blueF());
                     }
                     else
                         glColor3f(vertexColors[id].redF(),vertexColors[id].greenF(), vertexColors[id].blueF());

                 }
                 else
                   glColor3f(1.0,1.0,1.0);

                 glNormal3dv(&(*fi).V(i)->N()[0]);
                 glVertex3dv(&(*fi).V(i)->P()[0]);
             }
             faceIdx++;
         }
        glEnd();

    }
            */
}

 void GLWidget::draw()
 {
	//escena->skinner->computeDeformationsWithSW(escena->skeletons);
	for(int j = 0; j < escena->skeletons.size(); j++) 
	{
		escena->skeletons[j]->root->computeWorldPos();
	}

	AirRig* rig = (AirRig*)escena->rig;

	if(rig) 
	{
		if(rig->enableDeformation && !rig->rigginMode)
		{
			if(!rig->enableTwist)
				rig->airSkin->computeDeformations(rig);
			else
				rig->airSkin->computeDeformationsWithSW(rig);
		}
	}

	AdriViewer::draw();
 
	for(unsigned int i = 0; i< rig->defRig.defGroups.size(); i++)
     {
         ((DefGroupRender*)rig->defRig.defGroups[i]->shading)->drawFunc();
     }
 }

void GLWidget::drawModel()
{

}

void GLWidget::drawWithCages()
{
    //TODO
    /*

    MyMesh *currentModelo;
    MyMesh *currentCage;


    //if(!showDeformedModel)
    //{
    //    currentModelo = &modelo;
    //    currentCage = &cage;
    //}
    //else
    //{
    //    currentModelo = &newModelo;
    //    currentCage = &newCage;
    //}


    if(stillCageAbled && stillCageSelected >= 0)
        currentCage = m.stillCages[stillCageSelected];
    else
        currentCage = &m.dynCage;

    if(!showDeformedModel)
        currentModelo = &m.modeloOriginal;
    else
    {
        if(activeCoords == HARMONIC_COORDS)
            currentModelo = &m.newModeloHC;
        else if(activeCoords == GREEN_COORDS)
            currentModelo = &m.newModeloGC;
        else
            currentModelo = &m.newModeloMVC;
    }


     //MyMesh::PerVertexAttributeHandle<unsigned int > vertIdxHnd =  tri::Allocator<MyMesh>::GetPerVertexAttribute<unsigned int >(*currentModelo,"vertIdx");

    glShadeModel(GL_SMOOTH);
    if(loadedModel)
    {
        //vector<float> normalsArray;
        //vector<float> vertexes;
        //vector<float> colors;
        */
        /*
        MyMesh::FaceIterator fi;
        int faceIdx = 0;
        for(fi = currentModelo->face.begin(); fi!=currentModelo->face.end(); ++fi )
         {
             //glNormal3dv(&(*fi).N()[0]);
             for(int i = 0; i<3; i++)
             {
                 if(shadeCoordInfluence || m_bDrawHCInfluences)
                 {
                     QColor color;
                     if(activeCoords == GREEN_COORDS && bGCcomputed)
                       color = GetColour(selectionValuesForColorGC[fi->V(i)->IMark()], maxMinGC[1],maxMinGC[0]);

                     else if(activeCoords == HARMONIC_COORDS && bHComputed)
                         color = GetColour(selectionValuesForColorHC[fi->V(i)->IMark()], maxMinHC[1],maxMinHC[0]);

                     else if(activeCoords == MEANVALUE_COORDS && bMVCComputed)
                         color = GetColour(selectionValuesForColorMVC[fi->V(i)->IMark()], maxMinMVC[1],maxMinMVC[0]);

                     else color = QColor(255,255,255);

                     colors.push_back(color.redF());
                     colors.push_back(color.greenF());
                     colors.push_back(color.blueF());

                 }
                 else
                 {
                     colors.push_back(1.0);
                     colors.push_back(1.0);
                     colors.push_back(1.0);
                 }

                 normalsArray.push_back((*fi).V(i)->N()[0]);
                 normalsArray.push_back((*fi).V(i)->N()[1]);
                 normalsArray.push_back((*fi).V(i)->N()[2]);

                 vertexes.push_back((*fi).V(i)->P()[0]);
                 vertexes.push_back((*fi).V(i)->P()[1]);
                 vertexes.push_back((*fi).V(i)->P()[2]);
             }
             faceIdx++;
         }


        //glBegin(GL_TRIANGLES);



        //glEnableClientState(GL_NORMAL_ARRAY);
        //glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);

        GLuint buffers[3];

        glGenBuffers(3, buffers);

          glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
          glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertexes.size(), &vertexes[0], GL_STATIC_DRAW);
          glVertexPointer(3, GL_FLOAT, sizeof(float)*3, BUFFER_OFFSET(0));

          //glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
          //glBufferData(GL_ARRAY_BUFFER, sizeof(float)*normalsArray.size(), &normalsArray[0], GL_STATIC_DRAW);
          //glVertexPointer(3, GL_FLOAT, sizeof(float)*3, BUFFER_OFFSET(0));

          //glBindBuffer(GL_ARRAY_BUFFER, buffers[2]);
          //glBufferData(GL_ARRAY_BUFFER, sizeof(float)*colors.size(), &colors[0], GL_STATIC_DRAW);
          //glVertexPointer(3, GL_FLOAT, sizeof(float)*3, BUFFER_OFFSET(0));


        glDrawArrays(GL_TRIANGLES, 0, vertexes.size()*sizeof(float));

        // deactivate vertex arrays after drawing
        glDisableClientState(GL_VERTEX_ARRAY);
        //glDisableClientState(GL_COLOR_ARRAY);
        //glDisableClientState(GL_NORMAL_ARRAY);

        glDeleteBuffers(3, &buffers[0]);
        */
        /*
        glColor3f(1.0, 1.0, 1.0);
        glColor3f(color.redF(),color.greenF(), color.());
        glNormal3dv(&(*fi).V(i)->N()[0]);
        glVertex3dv(&(*fi).V(i)->P()[0]);
        */
        //glEnd();

        /*
        // Pintamos en modo directo el modelo
        glBegin(GL_TRIANGLES);
        glColor3f(1.0, 1.0, 1.0);
        MyMesh::FaceIterator fi;
        int faceIdx = 0;
        for(fi = currentModelo->face.begin(); fi!=currentModelo->face.end(); ++fi )
         {
             //glNormal3dv(&(*fi).N()[0]);
             for(int i = 0; i<3; i++)
             {
                 if(shadeCoordInfluence || m_bDrawHCInfluences)
                 {
                     QColor color;
                     if(activeCoords == GREEN_COORDS && bGCcomputed)
                     {
                         color = GetColour(selectionValuesForColorGC[fi->V(i)->IMark()], maxMinGC[1],maxMinGC[0]);
                         //GetColour(selectionValuesForColorGC[fi->V(i)->IMark()], 0, 1);
                     }


                     else if(activeCoords == HARMONIC_COORDS && bHComputed)
                     {
                         //GetColour(selectionValuesForColorHC[fi->V(i)->IMark()], 0, 1);
                         color = GetColour(selectionValuesForColorHC[fi->V(i)->IMark()], maxMinHC[1],maxMinHC[0]);
                     }

                     else if(activeCoords == MEANVALUE_COORDS && bMVCComputed)
                     {
                         //GetColour(selectionValuesForColorMVC[fi->V(i)->IMark()], -1, 1);
                         color = GetColour(selectionValuesForColorMVC[fi->V(i)->IMark()], maxMinMVC[1],maxMinMVC[0]);
                     }

                     else color = QColor(255,255,255);

                     glColor3f(color.redF(),color.greenF(), color.blueF());
                 }
                 else
                     glColor3f(1.0, 1.0, 1.0);

                 glNormal3dv(&(*fi).V(i)->N()[0]);
                 glVertex3dv(&(*fi).V(i)->P()[0]);
             }
             faceIdx++;
         }
        glEnd();



        if(drawCage)
        {
            glDisable(GL_LIGHTING);
            glBegin(GL_LINES);
            glColor3f(0.1, 0.1, 1.0);
            MyMesh::FaceIterator fi;
            for(fi = currentCage->face.begin(); fi!=currentCage->face.end(); ++fi )
                 {
                     for(int i = 0; i<3; i++)
                     {
                         glVertex3dv(&(*fi).P(i)[0]);
                         glVertex3dv(&(*fi).P((i+1)%3)[0]);
                     }
                 }
            glEnd();


            glPointSize(5);
            glColor3f(0.1, 1.0, 0.1);

            int idx = 0;
            MyMesh::VertexIterator vi;
            for(vi = currentCage->vert.begin(); vi!=currentCage->vert.end(); ++vi )
            {
                glBegin(GL_POINTS);
                if(shadeCoordInfluence && selection_.contains(idx))
                {
                    glEnd();
                    glPointSize(12);
                    glColor3f(1.0, 0.1, 0.1);
                    glBegin(GL_POINTS);
                    glVertex3dv(&(*vi).P()[0]);
                    glEnd();
                    glPointSize(5);
                    glColor3f(0.1, 1.0, 0.1);
                    glBegin(GL_POINTS);
                }
                else
                    glVertex3dv(&(*vi).P()[0]);

                idx++;
                glEnd();
            }
           glEnable(GL_LIGHTING);
        }

        if(m_bShowHCGrid)
        {
            Vector3f greenCube(0.5,1.0,0.5);
            Vector3f redCube(1.0,0.5,0.5);
            Vector3f blueCube(0.5,0.5,1.0);

            if(m_bShowAllGrid)
            {
                for(int i = 0; i< m.HCgrid.dimensions.X(); i++)
                {
                    for(int j = 0; j< m.HCgrid.dimensions.Y(); j++)
                    {
                        for(int k = 0; k< m.HCgrid.dimensions.Z(); k++)
                        {
                            Vector3d o(m.HCgrid.bounding.min + Vector3d(i,j,k)*m.HCgrid.cellSize);

                            if(m_bShowHCGrid_interior && m.HCgrid.cells[i][j][k]->tipo == INTERIOR)
                                drawCube(o, m.HCgrid.cellSize, greenCube);

                            if(m_bShowHCGrid_exterior && m.HCgrid.cells[i][j][k]->tipo == EXTERIOR)
                                drawCube(o, m.HCgrid.cellSize, blueCube);

                            if(m_bShowHCGrid_boundary && m.HCgrid.cells[i][j][k]->tipo == BOUNDARY)
                            {
                                drawCube(o, m.HCgrid.cellSize, redCube);
                            }
                        }
                    }
                }
            }
            else
            {
                int j = currentDrawingSlice;
                for( int i = 0; i< m.HCgrid.dimensions.X(); i++)
                {
                    for( int k = 0; k< m.HCgrid.dimensions.Z(); k++)
                    {
                        Vector3d o(m.HCgrid.bounding.min + Vector3d(i,j,k)*m.HCgrid.cellSize);

                        if(m_bDrawHCInfluences)
                        {
                            glEnable(GL_BLEND);
                            if((unsigned int) i >= m.HCgrid.cells.size())
                                continue;

                            if(j < 0 || j >= (int)m.HCgrid.cells[i].size())
                                continue;

                            if((unsigned int) k >= m.HCgrid.cells[i][j].size())
                                continue;

                            QColor c;
                            c = GetColour(sliceValues[i][k],maxMinHC[1],maxMinHC[0]);


                            Vector3f color(c.redF(),c.greenF(), c.blueF());

                            if(m_bShowHCGrid_interior && m.HCgrid.cells[i][j][k]->tipo == INTERIOR)
                                drawCube(o, m.HCgrid.cellSize, color, m_bDrawHCInfluences);

                            if(m_bShowHCGrid_exterior && m.HCgrid.cells[i][j][k]->tipo == EXTERIOR)
                                drawCube(o, m.HCgrid.cellSize, color, m_bDrawHCInfluences);

                            if(m_bShowHCGrid_boundary && m.HCgrid.cells[i][j][k]->tipo == BOUNDARY)
                            {
                                drawCube(o, m.HCgrid.cellSize, color, m_bDrawHCInfluences);
                            }
                            glDisable(GL_BLEND);
                        }
                        else
                        {
                            if(m_bShowHCGrid_interior && m.HCgrid.cells[i][j][k]->tipo == INTERIOR)
                                drawCube(o, m.HCgrid.cellSize, greenCube);

                            if(m_bShowHCGrid_exterior && m.HCgrid.cells[i][j][k]->tipo == EXTERIOR)
                                drawCube(o, m.HCgrid.cellSize, blueCube);

                            if(m_bShowHCGrid_boundary && m.HCgrid.cells[i][j][k]->tipo == BOUNDARY)
                            {
                                drawCube(o, m.HCgrid.cellSize, redCube);
                            }
                        }
                    }
                }
            }
        }
    }

    // Draws manipulatedFrame (the set's rotation center)
    if (selection_.size()>0)
      {
        glPushMatrix();
        glMultMatrixd(manipulatedFrame()->matrix());
        drawAxis(0.5);
        glPopMatrix();
      }

    // Draws rectangular selection area. Could be done in postDraw() instead.
    if (selectionMode_ != NONE)
      drawSelectionRectangle();

      */
}

void GLWidget::ChangeViewingMode(int)
{

}



void GLWidget::ChangeSliceXZ(int slice)
{
    updateGridRender();
}

void GLWidget::ChangeSliceXY(int slice)
{
   updateGridRender();

   //TODO
   /*
   currentDrawingSlice = slice;

   if(!bHComputed)
       return;

   sliceValues.resize(m.HCgrid.dimensions.X());

   for(unsigned int i = 0; i< sliceValues.size(); i++)
   {
       sliceValues[i].resize(m.HCgrid.dimensions.Z());
       for(unsigned int j = 0; j< sliceValues[i].size(); j++)
       {
           sliceValues[i][j] = 0;
       }
   }

   for(unsigned int i = 0; i< sliceValues.size(); i++)
   {
       for(unsigned int j = 0; j< sliceValues[i].size(); j++)
       {
           float valueSum = 0;
           for (QList<int>::const_iterator it=selection_.begin(), end=selection_.end(); it != end; ++it)
           {
               valueSum += m.HCgrid.cells[i][currentDrawingSlice][j]->weights[*it];
           }

           if (valueSum != 0)
           {
               sliceValues[i][j] = valueSum;
               //printf("Valor de %d %d : %f", i, j, sliceValues[i][j]); fflush(0);
           }
       }
   }
   */

}

void GLWidget::saveScene(string fileName, string name, string path, bool compactMode)
 {
     QFile modelDefFile(fileName.c_str());
     if(modelDefFile.exists())
     {
		modelDefFile.open(QFile::WriteOnly);
        QTextStream out(&modelDefFile);

		out << escena->sSceneName.c_str() << endl << endl;
		out << escena->sGlobalPath.c_str() << endl << endl ;
		out << escena->sPath.c_str() << endl << endl ;

		if(escena->models.size() == 0 )
			return;

		int modelFlag = 0, sktFlag = 0, embeddingFlag = 0, bindingFlag = 0, rigFlag = 0;

		if(escena->models.size() > 0) modelFlag = 1;
		if(escena->skeletons.size() > 0) sktFlag = 1;
		embeddingFlag = 1;
		if(escena->rig) 
		{
			rigFlag = 1;
			if(escena->rig->skin->bind) bindingFlag = 1;
		}

		out << "## Flags: model, skeleton, embedding, binding, grid, rig" << endl ;
		out << modelFlag << " " << sktFlag << " " << embeddingFlag << " " << bindingFlag << " " << rigFlag << endl << endl;

		QString sSaveRigFile, sSaveBindingFile;

		if(modelFlag)
		{
			out << "## Modelo" << endl ;
			out << escena->sSceneName.c_str() << ".off" << endl << endl;
		}

		if(sktFlag)
		{
			out << "## Esqueletos" << endl ;
			out << escena->sSceneName.c_str() << "_skeleton.txt" << endl << endl;
		}

		if(embeddingFlag)
		{
			out << "## Embedding" << endl ;
			out << escena->sSceneName.c_str() << "_embeding.txt" << endl << endl;
		}

		if(bindingFlag)
		{
			out << "## Point weights" << endl ;
			sSaveBindingFile = (escena->sSceneName + "_binding.txt").c_str();
			out << sSaveBindingFile << endl << endl;

			sSaveBindingFile = (escena->sGlobalPath + escena->sPath + sSaveBindingFile.toStdString()).c_str();
		}

		if(rigFlag)
		{
			out << "## Rig" << endl ;
			sSaveRigFile = (escena->sSceneName + "_rig.txt").c_str();
			out << sSaveRigFile << endl << endl;

			sSaveRigFile = (escena->sGlobalPath + escena->sPath + sSaveRigFile.toStdString()).c_str();

		}

		out << fileName.c_str();
		modelDefFile.close();

		if(rigFlag)
		{
			AirRig* rig = (AirRig*)escena->rig;

			// Save Rigging
			FILE* fout = fopen(sSaveRigFile.toStdString().c_str(), "w");
			if(fout) 
			{
				rig->saveToFile(fout);
				fclose(fout);
			}
			
			// Save Skinning
			if(compactMode)
			{
				// Build the deformers name map.
				map<int, string> deformersNamesMap;

				for(int sktIdx = 0; sktIdx < rig->defRig.defGroups.size(); sktIdx++)
				{
					deformersNamesMap[rig->defRig.defGroups[sktIdx]->nodeId] = rig->defRig.defGroups[sktIdx]->transformation->sName;
				}
				rig->skin->bind->saveCompactBinding(sSaveBindingFile.toStdString(), deformersNamesMap);
				saveAirBinding(rig->skin->bind, (sSaveBindingFile.append("_extend.txt")).toStdString());
			}
			else
				saveAirBinding(rig->skin->bind, sSaveBindingFile.toStdString());
		}
    }
 }

 void GLWidget::readScene(string fileName, string name, string path)
 {
     QFile modelDefFile(fileName.c_str());
     if(modelDefFile.exists())
     {
        modelDefFile.open(QFile::ReadOnly);
        QTextStream in(&modelDefFile);

        QString sSceneName = in.readLine(); in.readLine();
        QString sGlobalPath = in.readLine(); in.readLine();
        QString sPath = in.readLine(); in.readLine(); in.readLine();

		escena->sSceneName = sSceneName.toStdString();
		escena->sGlobalPath = sGlobalPath.toStdString();
		escena->sPath = sPath.toStdString();

		QString sModelFile = "", sSkeletonFile = "", sEmbeddingFile = "", sBindingFile = "", sRiggingFile = "";

		QStringList flags = in.readLine().split(" "); in.readLine(); in.readLine();
		if(flags.size() != 5)
			return;

		if(flags[0].toInt() != 0 && !in.atEnd())
		{
			sModelFile = in.readLine(); in.readLine(); in.readLine();
		}

		if(flags[1].toInt() != 0 && !in.atEnd())
		{
			sSkeletonFile = in.readLine(); in.readLine(); in.readLine();
		}

		if(flags[2].toInt() != 0 && !in.atEnd())
		{
			sEmbeddingFile = in.readLine(); in.readLine(); in.readLine();
		}

		if(flags[3].toInt() != 0 && !in.atEnd())
		{
			sBindingFile = in.readLine(); in.readLine(); in.readLine();
		}

		if(flags[4].toInt() != 0 && !in.atEnd())
		{
			sRiggingFile = in.readLine(); in.readLine(); in.readLine();
		}

		modelDefFile.close();

		QString newPath( path.c_str());
        newPath = newPath +"/";
        if(!sPath.isEmpty())
            newPath = newPath+"/"+sPath +"/";

        // Leer modelo
        readModel( (newPath+sModelFile).toStdString(), sSceneName.toStdString(), newPath.toStdString());

		// Constuir datos sobre el modelo
        Modelo* m = ((Modelo*)escena->models.back());
        m->sPath = newPath.toStdString(); // importante para futuras referencias
		BuildSurfaceGraphs(m);
		getBDEmbedding(m);

        // Leer esqueleto
		if(!sSkeletonFile.isEmpty())
		{
			string sSkeletonFileFullPath = (newPath+sSkeletonFile).toStdString();
			readSkeletons(sSkeletonFileFullPath, escena->skeletons);
		}

		bool skinLoaded = false, riggLoaded = false;

		// Skinning
		if(!sBindingFile.isEmpty())
		{
			string sBindingFileFullPath = (newPath+sBindingFile).toStdString();//path.toStdString();
			
			//Copia del modelo para poder hacer deformaciones
			if(!m->originalModelLoaded)
				initModelForDeformation(m);
			
			QString extendedBinding = sBindingFileFullPath.c_str();
			extendedBinding.append("_extend.txt");

			// Cargar en memoria los datos del binding tal cual
			loadAirBinding(m->bind, extendedBinding.toStdString());
			skinLoaded = true;
		}

		// Rigging
		if(!sRiggingFile.isEmpty())
		{
			// Load from disc
			string sRiggingFileFullPath = (newPath+sRiggingFile).toStdString();//path.toStdString();
			((AirRig*)escena->rig)->loadRigging(sRiggingFileFullPath);	
			riggLoaded = true;
		}


		if(riggLoaded)
		{
			// By now is with the skeleton, but soon will be alone
			((AirRig*)escena->rig)->bindLoadedRigToScene(m, escena->skeletons);
		}

		if(skinLoaded)
		{
			// Now it's time to do a correspondence with the loaded data and the scene.
			//((AirRig*)escena->rig)->airSkin->loadBindingForModel(m, (AirRig*)escena->rig);

			((AirRig*)escena->rig)->airSkin->computeRestPositions(escena->skeletons);

			((AirRig*)escena->rig)->enableDeformation = true;
		}

		paintModelWithData();

    }
 }