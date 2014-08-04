#include "GLWidget.h"
#include "AppMgr.h"

#include <ui_mainwindow.h>
#include <QtCore/QTextStream>
#include <QtCore/qdir.h>

#include <render/graphRender.h>
#include <render/defGorupRender.h>

#include <utils/util.h>

#include <Computation\BiharmonicDistances.h>
#include <Computation\mvc_interiorDistances.h>
#include <Computation\AirSegmentation.h>
#include <Computation\mvc_interiorDistances.h>
#include <computation\HarmonicCoords.h>

#include <DataStructures\InteriorDistancesData.h>
#include <DataStructures\DataStructures.h>

#include <DataStructures\AirVoxelization.h>

#include <omp.h>
#include <Eigen/Core>
//#include <tetgen/tetgen.h>
#include <chrono>

using namespace std::chrono;
using namespace Eigen;

#define debugingSelection false

GLWidget::GLWidget(QWidget * parent, const QGLWidget * shareWidget,
                   Qt::WindowFlags flags ) : AdriViewer(parent, shareWidget, flags) {
    
	// TOREMOVE
	drawCage = true;
    shadeCoordInfluence = true;

    influenceDrawingIdx = 0;

    stillCageAbled = false;
    stillCageSelected = -1;

	valueAux = 0;

	// Init rigg variables
	if(escena->rig)
		delete escena->rig;

	escena->rig = new AirRig(scene::getNewId());

	// TODEBUG_WORKERS
	//worker.rig = (AirRig*)escena->rig;

	// Verbose flags
	bDrawStatistics = false;
	showBulgeInfo = false;

	sktCr = new sktCreator();

	m_currentShader = NULL;
	preferredType = SHD_VERTEX_COLORS;
	preferredNormalType = SHD_VERTEX_COLORS;

	X_ALT_modifier = false;

	setAutoFillBackground(false);

	last_time = 0;

	m_bRTInteraction = true;

}


void GLWidget::updateColorBufferObject()
{

}

void GLWidget::updateVertexBufferObject()
{

}


void GLWidget::prepareVertexBufferObject()
{

	int numVertexes = 3;
	vector<float> positionData;
	vector<float> colorData;

	// vertex positions
	m_vertexPositionBuffer.create();
	m_vertexPositionBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
	m_vertexPositionBuffer.bind();
	m_vertexPositionBuffer.allocate(positionData.data(), numVertexes*3*sizeof(float));

	// vertex colors
	m_vertexColorBuffer.create();
	m_vertexColorBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
	m_vertexColorBuffer.bind();
	m_vertexColorBuffer.allocate(colorData.data(), numVertexes*3*sizeof(float));

	// make this shader de active one
	m_currentShader->bind();

	//bind position buffer
	m_vertexPositionBuffer.bind(); // make it positions as current
	m_currentShader->enableAttributeArray("vertexPosition");
	m_currentShader->setAttributeBuffer("vertexPosition", GL_FLOAT, 0, 3);

	// bind Color buffer
	m_vertexColorBuffer.bind(); // make it positions as current
	m_currentShader->enableAttributeArray("vertexColor");
	m_currentShader->setAttributeBuffer("vertexColor", GL_FLOAT, 0, 3);

}

void loadShaderAndLink(QOpenGLShaderProgram* shp, string vertFile, string fragFile)
{
	if(!shp->addShaderFromSourceFile(QOpenGLShader::Vertex, vertFile.c_str()))
	{
		printf("No puedo usar el vertex shader. Log:%s", shp->log().toStdString().c_str()); fflush(0);
	}

	if(!shp->addShaderFromSourceFile(QOpenGLShader::Fragment, fragFile.c_str()))
	{
		printf("No puedo usar el fragment shader. Log:%s", shp->log().toStdString().c_str()); fflush(0);
	}

	if(!shp->link())
	{
		printf("No puedo lincar. Log:%s", shp->log().toStdString().c_str()); fflush(0);
	}
}



void GLWidget::setShaderConfiguration( shaderIdx type)
{
	if(type == SHD_XRAY) // XRAY
	{
		m_currentShader = m_shaders[SHD_XRAY];
		m_currentShader->bind();

		// default lighting properties
		m_currentShader->setUniformValue("light.position", QVector4D(5.0f, 5.0f, 5.0f, 1.0f));
		m_currentShader->setUniformValue("light.intensity", QVector3D(1.0f, 1.0f, 1.0f));

		// Default matrial properties
		m_currentShader->setUniformValue("material.ka", QVector3D(0.1f, 0.8f, 0.0f));
		m_currentShader->setUniformValue("material.kd", QVector3D(0.75f, 0.95f, 0.75f));
		m_currentShader->setUniformValue("material.ks", QVector3D(0.2f, 0.2f, 0.2f));
		m_currentShader->setUniformValue("material.shininess", 100.0f);
		m_currentShader->setUniformValue("material.opacity", 1.0f);
	}
	else if(type == SHD_NO_SHADE)
	{
		m_currentShader = m_shaders[SHD_NO_SHADE];
		m_currentShader->bind();

		m_currentShader->setUniformValue("material.ka", QVector3D(1.0f, 1.0f, 1.0f));
		m_currentShader->setUniformValue("material.shininess", 100.0f);
		m_currentShader->setUniformValue("material.opacity", 1.0f);
	}
	else if(type == SHD_VERTEX_COLORS)
	{
		m_currentShader = m_shaders[SHD_VERTEX_COLORS];
		m_currentShader->bind();

		// default lighting properties
		m_currentShader->setUniformValue("light.position", QVector4D(5.0f, 5.0f, 5.0f, 1.0f));
		m_currentShader->setUniformValue("light.intensity", QVector3D(1.0f, 1.0f, 1.0f));
		m_currentShader->setUniformValue("material.ka", QVector3D(1.0f, 1.0f, 1.0f));
		m_currentShader->setUniformValue("material.kd", QVector3D(1.0f, 1.0f, 1.0f));
		m_currentShader->setUniformValue("material.ks", QVector3D(1.0f, 1.0f, 1.0f));
		m_currentShader->setUniformValue("material.shininess", 100.0f);
		m_currentShader->setUniformValue("material.opacity", 1.0f);
	}
	else
	{
		m_currentShader = m_shaders[SHD_BASIC];
		m_currentShader->bind();

		// default lighting properties
		m_currentShader->setUniformValue("light.position", QVector4D(5.0f, 5.0f, 5.0f, 1.0f));
		m_currentShader->setUniformValue("light.intensity", QVector3D(1.0f, 1.0f, 1.0f));

		// Default matrial properties
		m_currentShader->setUniformValue("material.ka", QVector3D(0.15f, 0.15f, 0.15f));
		m_currentShader->setUniformValue("material.kd", QVector3D(0.65f, 0.65f, 0.65f));
		m_currentShader->setUniformValue("material.ks", QVector3D(0.1f, 0.1f, 0.1f));
		m_currentShader->setUniformValue("material.shininess", 200.0f);
		m_currentShader->setUniformValue("material.opacity", 1.0f);
	}

	m_currentype = type;
}

void GLWidget::prepareShaderProgram()
{
	// Shader phong std 
	m_shaders.resize(SHD_LENGTH);
	
	m_shaders[SHD_BASIC] = new QOpenGLShaderProgram();
	printf("Basic shader loading\n");
	loadShaderAndLink(m_shaders[SHD_BASIC],
					  "C:/dev/air/AirLib/shaders/phong.vert",
					  "C:/dev/air/AirLib/shaders/phong.frag");

	m_shaders[SHD_XRAY] = new QOpenGLShaderProgram();
	printf("XRay loading\n");
	loadShaderAndLink(m_shaders[SHD_XRAY],
					  "C:/dev/air/AirLib/shaders/xray.vert",
					  "C:/dev/air/AirLib/shaders/xray.frag");

	m_shaders[SHD_VERTEX_COLORS] = new QOpenGLShaderProgram();
	printf("VertexColor loading\n");
	loadShaderAndLink(m_shaders[SHD_VERTEX_COLORS],
					  "C:/dev/air/AirLib/shaders/phong.vert",
					  "C:/dev/air/AirLib/shaders/phong_vertexColors.frag");


	m_shaders[SHD_NO_SHADE] = new QOpenGLShaderProgram();
	printf("No shade loading\n");
	loadShaderAndLink(m_shaders[SHD_NO_SHADE],
					  "C:/dev/air/AirLib/shaders/phong.vert",
					  "C:/dev/air/AirLib/shaders/noLight_vertexColors.frag");


	m_currentShader = m_shaders[SHD_BASIC];
}

void GLWidget::init()
{
	//setHandlerKeyboardModifiers(QGLViewer::SELECT, Qt::KeyboardModifier::NoModifier);
	//setHandlerKeyboardModifiers(QGLViewer::CAMERA, Qt::KeyboardModifier::AltModifier);
	//setHandlerKeyboardModifiers(QGLViewer::FRAME, Qt::KeyboardModifier::NoModifier);
	//setHandlerKeyboardModifiers(QGLViewer::CAMERA, Qt::KeyboardModifier::ControlModifier);
	//setMouseBinding(Qt::NoModifier, Qt::MouseButton::LeftButton, NO_CLICK_ACTION);
	//setMouseBinding(Qt::ControlModifier | Qt::ShiftModifier, NO_CLICK_ACTION, false, Qt::LeftButton);

	//setMouseBinding(Qt::ControlModifier | Qt::ShiftModifier | Qt::RightButton, SELECT);
	
	//setMouseBinding(Qt::AltModifier | Qt::LeftButton , NO_CLICK_ACTION);
	//setMouseBinding(Qt::LeftButton, NO_CLICK_ACTION);

	/*
		setMouseBinding(Qt::ControlModifier | Qt::LeftButton, CAMERA, NO_MOUSE_ACTION);
		setMouseBinding(Qt::ControlModifier | Qt::LeftButton, NO_CLICK_ACTION);

		setMouseBinding(Qt::LeftButton, CAMERA, NO_MOUSE_ACTION);
		setMouseBinding(Qt::LeftButton, NO_CLICK_ACTION);

		setMouseBinding(Qt::ControlModifier, CAMERA, NO_MOUSE_ACTION);
		setMouseBinding(Qt::ControlModifier, NO_CLICK_ACTION);

		setMouseBinding(Qt::ControlModifier | Qt::LeftButton, CAMERA, ROTATE);
	*/

	//setMouseBinding(Qt::LeftButton, CAMERA, ROTATE);

	prepareShaderProgram();
	setMouseTracking(true);

	AdriViewer::init();
}

/*
 void GLWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Save current OpenGL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Reset OpenGL parameters
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_MULTISAMPLE);
    static GLfloat lightPosition[4] = { 1.0, 5.0, 5.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    
	qglClearColor(QColor(Qt::black));
	
    // Classical 3D drawing, usually performed by paintGL().
    preDraw();
    draw();
    postDraw();
    
	// Restore OpenGL state
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
	
	drawOverpaint(&painter, event);

    painter.end();
}

 void GLWidget::drawOverpaint(QPainter *painter,  QPaintEvent *event) 
 {
	QLinearGradient gradient(QPointF(50, -20), QPointF(80, 20));
    gradient.setColorAt(0.0, Qt::white);
    gradient.setColorAt(1.0, QColor(0xa6, 0xce, 0x39));

    QBrush circleBrush = QBrush(gradient);
    QPen circlePen = QPen(Qt::blue); circlePen.setWidth(1);
    QPen textPen = QPen(Qt::white);
    QFont textFont; textFont.setPixelSize(50);

	QBrush background = QBrush(QColor(64, 32, 64));
	painter->fillRect(event->rect(), background);
    painter->translate(100, 100);

	int elapsed = 0;

	painter->setBrush(circleBrush);
    painter->setPen(circlePen);
    painter->rotate(elapsed * 0.030);

    qreal r = elapsed / 1000.0;
    int n = 30;
    for (int i = 0; i < n; ++i) {
        painter->rotate(30);
        qreal factor = (i + r) / n;
        qreal radius = 0 + 120.0 * factor;
        qreal circleRadius = 1 + factor * 20;
        painter->drawEllipse(QRectF(radius, -circleRadius,
                                    circleRadius * 2, circleRadius * 2));
    }
    
	painter->restore();
	painter->setPen(textPen);
    painter->setFont(textFont);
    painter->drawText(QRect(-50, -50, 100, 100), Qt::AlignCenter, QStringLiteral("Qt"));
	return;


	painter->save();
	int CX = width()/2;
	int CY = height()/2;
    painter->translate(CX, CY);

	QRadialGradient radialGrad(QPointF(-40, -40), 100);
	radialGrad.setColorAt(0, QColor(255, 255, 255, 100));
	radialGrad.setColorAt(1, QColor(200, 200, 0, 100)); 
    
	//painter->setBrush(QBrush(radialGrad));

	painter->setBrush(QBrush(QColor(Qt::blue)));
	painter->drawRoundRect(-100, -100, 200, 200);
    painter->restore();
}

*/

void GLWidget::setTool(ToolType ctx)
{
	// activamos la opcion de creation
	if(ctx == CTX_TRANSFORMATION)
	{
		// Estabamos en el proceso de creación y hemos cortado
		if(selMgr.currentTool == CTX_CREATE_SKT && sktCr->state != SKT_CR_IDDLE)
			finishRiggingTool();

		//sktCr->dynRig->clear();
		selMgr.currentTool = ctx;
		preferredType = preferredNormalType;

		setCursor(Qt::ArrowCursor);
	}
	else if(ctx == CTX_CREATE_SKT)
	{
		//sktCr->dynRig->clear();
		setContextMode(CTX_SELECTION);
		selMgr.currentTool = ctx;
		sktCr->mode = SKT_RIGG;

		AirRig::mode = MODE_RIG;
		emit setRiggingToolUI();

		// TODO: Emitir signal para poner en rigg, no en anim

		preferredType = (SHD_XRAY);
		setCursor(Qt::CrossCursor);
	}
}

void cleanRigDefinition(AirRig* rig)
{
	for (int i = 0; i < rig->defRig.defGroups.size(); i++)
	{
		delete rig->defRig.defGroups[i];
	}

	rig->defRig.defGroups.clear();
	rig->defRig.defNodesRef.clear();
	rig->defRig.defGroupsRef.clear();
	rig->defRig.roots.clear();
}

void GLWidget::setToolCrtMode(int ctx)
{
	if(selMgr.currentTool == CTX_CREATE_SKT && sktCr->state != SKT_CR_IDDLE && sktCr->mode == SKT_CREATE)
		finishRiggingTool();
	
	AirRig* currentRig = (AirRig*)escena->rig;
	if(!sktCr->parentRig) sktCr->parentRig = currentRig;


	if(ctx == SKT_CREATE)
	{
		// Restore poses over all the defGroups
		currentRig->restorePoses();

		// En el caso de que haya algo seleccionado...
		if (selMgr.selection.size() > 0)
		{
			unsigned int nodeId = selMgr.selection[0]->nodeId;
			if (selMgr.selection[0]->iam == DEFGROUP_NODE)
			{
				sktCr->parentNode = currentRig->defRig.defGroupsRef[nodeId];

				if (sktCr->dynRig->defRig.defGroups.size() == 0)
					sktCr->addNewNode(sktCr->parentNode->getTranslation(false));
			}
		}
		else sktCr->parentNode = NULL;

		/*
		// Restore deformation to rest pose
		if(!currentRig->enableTwist)
			currentRig->airSkin->computeDeformations(currentRig);
		else
			currentRig->airSkin->computeDeformationsWithSW(currentRig);
			*/
		currentRig->airSkin->resetDeformations();

		currentRig->enableDeformation = false;

		if (!sktCr->parentNode)
			sktCr->state = SKT_CR_IDDLE;
		else
			sktCr->state = SKT_CR_SELECTED;

		// TO_UNCOMMENT
		//preferredType = SHD_XRAY;
		AirRig::mode = MODE_CREATE;
		setCursor(Qt::CrossCursor);
	}
	if(ctx == SKT_RIGG)
	{
		cleanRigDefinition(sktCr->dynRig);

		((AirRig*)escena->rig)->restorePoses();
		
		// No esta inicializado todavia.
		if(! currentRig->airSkin ) return;

		currentRig->airSkin->resetDeformations();
				
		// Restore manipulator
		if(selMgr.selection.size() > 0)
		{
			if(selMgr.selection[0]->iam == DEFGROUP_NODE)
			{
				DefGroup* dg = (DefGroup*)selMgr.selection[0];
				ToolManip.setFrame(dg->getTranslation(false), dg->getRotation(false), Vector3d(1,1,1));
			}
		}

		((AirRig*)escena->rig)->enableDeformation = false;


		sktCr->state = SKT_CR_IDDLE;
		// TO_UNCOMMENT
		//preferredType = SHD_XRAY;
		AirRig::mode = MODE_RIG;
		setCursor(Qt::ArrowCursor);
	}
	if(ctx == SKT_ANIM)
	{
		cleanRigDefinition(sktCr->dynRig);

		if(sktCr->mode != SKT_ANIM && sktCr->mode != SKT_TEST)
		{
			//Guardamos las poses, como poses de reposo.
			((AirRig*) escena->rig)->saveRestPoses();

			computeWeights();

			// disable deormation until the weights are computed
			((AirRig*)escena->rig)->enableDeformation = true;
		}

		sktCr->state = SKT_CR_IDDLE;
		preferredType = preferredNormalType;

		AirRig::mode = MODE_ANIM;
		setCursor(Qt::ArrowCursor);

	}
	else if(ctx == SKT_TEST)
	{
		cleanRigDefinition(sktCr->dynRig);

		//((AirRig*)escena->rig)->saveRestPoses();
		((AirRig*)escena->rig)->enableDeformation = true;

		sktCr->state = SKT_CR_IDDLE;
		preferredType = preferredNormalType;

		AirRig::mode = MODE_TEST;
		setCursor(Qt::ArrowCursor);
	}

	sktCr->mode = (sktToolMode)ctx;
}

void GLWidget::setContextMode(contextMode ctx)
 {
	if(ctx == CTX_SELECTION)
	{
		ToolManip.type = MANIP_NONE;
		selMgr.toolCtx = ctx;
	}
	else if(ctx == CTX_MOVE)
	{
		ToolManip.type = MANIP_MOVE;
		selMgr.toolCtx = ctx;
	}
	else if(ctx == CTX_ROTATION)
	{
		ToolManip.type = MANIP_ROT;
		selMgr.toolCtx = ctx;
	}
	else if(ctx == CTX_SCALE)
	{
		ToolManip.type = MANIP_SCL;
		selMgr.toolCtx = ctx;
	}
	else
	{
		printf("No conozco este contexto.\n");
	}
 }

 void GLWidget::endContextMode(contextMode ctx)
 {
	// limpiamos la opcion de creacion
	if(ctx == CTX_CREATE_SKT)
	{
		delete sktCr;
		sktCr = NULL;
	}
 }
 

void GLWidget::LaunchTests()
{
	vector<string> meshes;   
	string result; 

	string prefix = "C:/Users/chus/Documents/dev/Data/models/22_harry_pieces/closed_pieces/";

	for(int i = 0; i< 35; i++)
		meshes.push_back(QString("%1h%2.off").arg(prefix.c_str()).arg(i,2,10,QChar('0')).toStdString());

	result = prefix + "toda_la_maya.off";

	MergeMeshes(meshes, result);

	return;
}

void GLWidget::doTests(string fileName, string name, string path) 
{
	LaunchTests();
	return;

	// Load 2 cylinders, with colored faces.


	//assert(false);
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

void GLWidget::paintModelWithData() 
{

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

        double maxError = -9999;
        if(!m->bind) continue;

		int minId = -1, maxId = -1;

		#pragma omp parallel for
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
                newvalue = (pd.segmentId-FIRST_DEFNODE_ID);
                value = ((float)(newvalue))/((float)rig->defRig.defNodesRef.size()-1);

				if(minId < 0) minId = newvalue; // first time
				else minId = min(minId, newvalue);

				if(maxId < 0) maxId = newvalue; // first time
				else maxId = max(maxId, newvalue);
            }
            else if(escena->iVisMode == VIS_BONES_SEGMENTATION)
            {
				if(rig->defRig.defNodesRef.find(pd.segmentId) != rig->defRig.defNodesRef.end())
				{
					newvalue = rig->defRig.defNodesRef[pd.segmentId]->boneId-FIRST_DEFGROUP_ID;
					value = ((float)(newvalue))/((float)rig->defRig.defGroupsRef.size()-1);
				}
				else
				{
					value = 1.0;
				}
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
                value = 0.0;

                int searchedindex = -1;
                for(unsigned int ce = 0; ce < pd.influences.size(); ce++)
                {
                    if(pd.influences[ce].label == actualDesiredNumber)
                    {
                        searchedindex = ce;
                        break;
                    }
                }
                if(searchedindex >= 0)
                        value = pd.influences[searchedindex].weightValue;
            }
            else if(escena->iVisMode == VIS_SECW_WIDE)
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
                }
                if(searchedindex >= 0)
				{
					if (pd.secondInfluences.size() > 0 && pd.secondInfluences[searchedindex].size()> 0)
					{
						if(valueAux < pd.secondInfluences[searchedindex].size())
							value = pd.secondInfluences[searchedindex][valueAux].wideBone;
						else
							value = pd.secondInfluences[searchedindex][pd.secondInfluences[searchedindex].size()-1].wideBone;
					}
					else
					{
						value = 1.0;
					}
				}
            }
            else if(escena->iVisMode == VIS_SECW_ALONG)
            {
				value = 0.0;
				int actualDesiredNumber = escena->desiredVertex;		
				for(int groupIdx = 0; groupIdx < rig->defRig.defGroups.size(); groupIdx++)
				{
					// parche para video
					if(rig->defRig.defGroups[groupIdx]->relatedGroups.size() == 0) continue;

					if(rig->defRig.defGroups[groupIdx]->relatedGroups[0]->nodeId == actualDesiredNumber)
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
							value = pd.secondInfluences[searchedindex][valueAux].alongBone;
						else
							value = pd.secondInfluences[searchedindex][pd.secondInfluences[searchedindex].size()-1].alongBone;
					}
					else
					{
						value = 1.0;
					}
				}

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
                        value = m->bind->BihDistances[0].get(count,escena->desiredVertex) / maxdistance;
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
							value = pd.secondInfluences[searchedindex][valueAux].alongBone*
									pd.influences[searchedindex].weightValue;
						else
							value = pd.secondInfluences[searchedindex][pd.secondInfluences[searchedindex].size()-1].alongBone*
									pd.influences[searchedindex].weightValue;
					}
					else
					{
						value = 1.0*pd.influences[searchedindex].weightValue;
					}
				}

            }
            else if(escena->iVisMode == VIS_WEIGHT_INFLUENCE)
            {
                int modelVert = m->bind->pointData[count].node->id;
                value = weights[modelVert];
            }
            else
            {
                value = 1.0;
            }

            float r,g,b;
            GetColourGlobal(value,0.0,1.0, r, g, b);
            //QColor c(r,g,b);

			//int surfIdx = bd->pointData[count].component;
			//int vertIdxOnPiece = bd->pointData[count].component;

			//int idx = 0;
			//if(surfIdx > 0 && surfIdx < bd->surfaces.size())
			//	idx = surfIdx;

			//if(count < bd->surfaces[idx].nodes.size())
			//{
				m->shading->colors[bd->pointData[count].node->id].resize(3);
				m->shading->colors[bd->pointData[count].node->id][0] = r;
				m->shading->colors[bd->pointData[count].node->id][1] = g;
				m->shading->colors[bd->pointData[count].node->id][2] = b;
			//}
        }

		//printf("Rango de valores de id: %d to %d de %d\n", minId, maxId, rig->defRig.defNodesRef.size());
    }
}

void GLWidget::selectElements(vector<unsigned int > lst)
{
	// model and skeleton
	AdriViewer::selectElements(lst);

	if( !escena || !escena->rig) return;

	// For model painting
	escena->desiredVertex = -1;

	//// rig selection ////
	AirRig* rig = (AirRig*)escena->rig;
	for(int i = 0; i< rig->defRig.roots.size(); i++)
		rig->defRig.roots[i]->select(false, rig->defRig.roots[i]->nodeId);

	for(unsigned int i = 0; i< lst.size(); i++)
	{
		if(rig->isSelectable(lst[i]))
		{
			// group selection
			for(int ii = 0; ii< rig->defRig.defGroups.size(); ii++)
				if(rig->defRig.defGroups[ii]->nodeId == lst[i])
					rig->defRig.defGroups[ii]->select(true, lst[i]); 
						// Importante porque sino deseleccionamos los defGroups hijos...

			// Interface updating
			float expansion = 0;
			bool twistEnabled = false;
			bool bulgeEnabled = false;
			int localSmoothPases = 0;
			float twistIni = 0;
			float twistFin = 1;

			string sName;
			rig->getDataFromDefGroup(lst[i], expansion, twistEnabled, bulgeEnabled, 
									localSmoothPases, twistIni, twistFin);

			rig->getNodeData(lst[i], sName);

			emit labelsFromSelectedUpdate(lst[i], sName);
			emit defGroupValueUpdate(expansion, twistEnabled, 
									 bulgeEnabled, localSmoothPases, twistIni, twistFin);

			// Model color updating
			escena->desiredVertex = lst[i];
		}
	}

	paintModelWithData();
}

//////////////////////////////////////////////
//             BULGE EFFECT           ////////
//////////////////////////////////////////////
void GLWidget::initBulges(AirRig* rig)
{
		
	AirSkinning* skin = rig->airSkin;

	//   Bulge effect inicialization
	skin->bulge.clear();
	map<int, int> assignedToBulgeGroup;

	int count= 0;

	for(int defIdx = 0; defIdx < rig->defRig.defGroups.size(); defIdx++)
	{
		DefGroup* dg = rig->defRig.defGroups[defIdx];

		if( dg->bulgeEffect )
		{	
			count++;

			// No se hace copia... espero que se inicie bien.
			skin->bulge.push_back(BulgeDeformer());

			// Get indexes of playing joints
			int currentJoint = rig->defRig.defGroups[defIdx]->nodeId;
			int fatherJoint = -1;
			if(rig->defRig.defGroups[defIdx]->dependentGroups.size() > 0)
				fatherJoint = rig->defRig.defGroups[defIdx]->dependentGroups[0]->nodeId;

			// Ponemos un grupo por el padre
			int fatherGroup = -1;
			if(fatherJoint > 0)
			{
				fatherGroup = skin->bulge.back().groups.size();
				skin->bulge.back().groups.push_back(new BulgeGroup());

				skin->bulge.back().groups[fatherGroup]->deformerId = fatherJoint;
				skin->bulge.back().groups[fatherGroup]->childDeformerId = currentJoint;
			
				for (int k = 0; k < skin->bind->pointData.size(); k++) 
				{ 
					bool founded1 = false;
					PointData& data = skin->bind->pointData[k];
					for (int kk = 0; kk < data.influences.size(); ++kk) // and check all joints associated to them
					{ 
						if(data.influences[kk].label == fatherJoint)
						{
							skin->bulge.back().groups[fatherGroup]->vertexGroup.push_back(k);
							break;
						}
					}	
				}

				skin->bulge.back().groups[fatherGroup]->defCurve = new wireDeformer();
				skin->bulge.back().groups[fatherGroup]->u.resize(skin->bulge.back().groups[fatherGroup]->vertexGroup.size(),0.0);
				skin->bulge.back().groups[fatherGroup]->w.resize(skin->bulge.back().groups[fatherGroup]->vertexGroup.size(),0.0);

				// parent direction
				skin->bulge.back().groups[fatherGroup]->direction = true;

			}
			else printf("Hay hay un joint marcado sin padre... no tiene sentido!\n");

			for(int bulgeChild = 0; bulgeChild < dg->relatedGroups.size(); bulgeChild++)
			{
				// revisar esta inicializacion
				int currentGroup = skin->bulge.back().groups.size();
				skin->bulge.back().groups.push_back(new BulgeGroup());
				
				skin->bulge.back().groups[currentGroup]->deformerId = currentJoint;
				skin->bulge.back().groups[currentGroup]->childDeformerId = rig->defRig.defGroups[defIdx]->relatedGroups[bulgeChild]->nodeId;

				for (int k = 0; k < skin->bind->pointData.size(); k++) 
				{ 
					PointData& data = skin->bind->pointData[k];
					for (int kk = 0; kk < data.influences.size(); ++kk) // and check all joints associated to them
					{
						if(data.influences[kk].label == currentJoint && data.influences[kk].weightValue > 0 /*|| data.influences[kk].label == fatherJoint*/)
						{
							skin->bulge.back().groups[currentGroup]->vertexGroup.push_back(k);
							break;
						}
					}	
				}

				// Now, just two deformers... but maybe we will share the deformation between more.
				skin->bulge.back().groups[currentGroup]->defCurve = new wireDeformer();

				// resize all the coords for computation
				skin->bulge.back().groups[currentGroup]->u.resize(skin->bulge.back().groups[currentGroup]->vertexGroup.size(), 0.0);
				skin->bulge.back().groups[currentGroup]->w.resize(skin->bulge.back().groups[currentGroup]->vertexGroup.size(), 1.0);
			}
		}
	}

	printf("Loaded %d bulges\n", count);
}

void GLWidget::updateSubdivisionParameter(float subdivision)
{
	AirRig* rig = (AirRig*)escena->rig;

	for (int defId = 0; defId < rig->defRig.defGroups.size(); defId++)
	{
		rig->defRig.defGroups[defId]->subdivisionRatio = subdivision;
	}
}

void GLWidget::computeWeights()
{
	// Get values from UI
	updateSubdivisionParameter(parent->ui->bonesSubdivisionRatio->text().toFloat());

	//printf("Calculando con %f de factor de subdivision\n", parent->ui->bonesSubdivisionRatio->text().toFloat());

	// 0. Vincular el modelo al esqueleto si no se ha hecho todavia
	AirRig* rig = (AirRig*) escena->rig;
	rig->initRigWithModel((Modelo*)escena->models[0]);

	// 1. Revisar el rigg y marcar el trabajo a hacer.
	//rig->getWorkToDo(worker.preprocessNodes, worker.segmentationNodes);

	// Create enough workers to work with this model.
	if(compMgr.size() != ((Modelo*)escena->models[0])->bind->surfaces.size())
	{
		compMgr.resize(((Modelo*)escena->models[0])->bind->surfaces.size());

		for(int surfIdx = 0; surfIdx < compMgr.size(); surfIdx++)
		{
			// Assign the rig
			compMgr[surfIdx].rig = rig;

			// Assign the model and bind for computations
			compMgr[surfIdx].setModelForComputations((Modelo*)escena->models[0], surfIdx);
		}
	}

	for(int surfIdx = 0; surfIdx < compMgr.size(); surfIdx++)
	{
		// Updates all the necessary nodes depending on the dirty flags
		compMgr[surfIdx].updateAllComputations();
	}

	rig->defRig.cleanDefNodes();

	// 3.1. Set-up default deformations -> TODELETE 
	//for(int i = 1; i < rig->defRig.defGroups.size()-1; i++)
	//rig->defRig.defGroups[1]-> bulgeEffect = true;
	
	// Bulge initialization over skinning computation
	//initBulges(rig);

	// 4. Actualizar la escena
	// enable deformations and render data
	rig->enableDeformation = true;

	paintModelWithData();
	emit updateSceneView();

}

void GLWidget::updateComputations()
{

}

void GLWidget::eigenMultiplication(int elements)
{
	/*
	clock_t Initini, Initfin;
	Initini = clock();
	Eigen::VectorXf weights(elements);
	for(int i = 0; i < weights.size(); i++)
	{
		weights(i) = (float)rand()/RAND_MAX;	
	}

	Eigen::MatrixXf A(elements, elements);
	for(int i = 0; i <  A.rows(); i++)
	{
		for(int j = 0; j <  A.cols(); j++)
		{
			 A(i,j) = (float)rand()/RAND_MAX;	
		}
	}

	vector<float> wieghtsOld(elements);
	vector<double> wieghtsOldDouble(elements);
	vector< vector<float> > AOld(elements);
	
	symMatrixLight BihDistances;
	BihDistances.resize(elements);

	for(int i = 0; i< elements; i++)
	{
		AOld[i].resize(elements);
		for(int j = 0; j< elements; j++)
		{
			AOld[i][j] = A(i,j);
			BihDistances.set(i,j,A(i,j));
		} 	

		wieghtsOld[i] = weights(i);
		wieghtsOldDouble[i] = weights(i);
	} 
	
	Initfin = clock();




	// Regular computation
	float sumCPU = 0;
	int64 stlSortStart1 = GetTimeMs();
	for(int temp = 0; temp< 10; temp++)
	{
		sumCPU = 0;
		for(int iElem = 0; iElem< elements; iElem++)
		{
			float sumCPUTemp = 0;

			for(int jElem = 0; jElem< elements; jElem++)
			{
				sumCPUTemp += BihDistances.get(iElem,jElem)*wieghtsOldDouble[jElem];
			}

			sumCPU+= sumCPUTemp*wieghtsOldDouble[iElem];
		}
	}
	int64 stlSortEnd1 = GetTimeMs();

	// Regular computation
	float sumCPU2 = 0;
	int64 stlSortStart2 = GetTimeMs();
	for(int temp = 0; temp< 10; temp++)
	{
		sumCPU = 0;
		for(int iElem = 0; iElem< elements; iElem++)
		{
			float sumCPUTemp = 0;

			for(int jElem = 0; jElem< elements; jElem++)
			{
				sumCPUTemp += AOld[iElem][jElem]*wieghtsOld[jElem];
			}

			sumCPU2+= sumCPUTemp*wieghtsOld[iElem];
		}
	}
	int64 stlSortEnd2 = GetTimeMs();

	//Eigen Computation
	float result = 0;
	int64 stlSortStart3 = GetTimeMs();
	for(int temp = 0; temp< 10; temp++)
	{
		result = weights.transpose()*A*weights;
	}
	int64 stlSortEnd3 = GetTimeMs();

	if(result > sumCPU || result > sumCPU2)
	{
		printf("Eigen mas lento\n");
	}

	map<float, int> vector_a_ordenar;

	//printf("Eigen computation: %f -> result: %f\n", ((double)(fin2-ini2))/CLOCKS_PER_SEC * 100, result);
	//fflush(0);
	
	//printf("Regular computation: %f -> sumCPU: %f\n", ((double)(fin-ini))/CLOCKS_PER_SEC * 100, sumCPU);

	double value0 = (((double)(Initfin-Initini))/CLOCKS_PER_SEC) * 100;
	double value1 = ((double)(stlSortEnd1-stlSortStart1))/10;
	double value2 = ((double)(stlSortEnd2-stlSortStart2))/10;
	double value3 = ((double)(stlSortEnd3-stlSortStart3))/10;
	printf("[%d](%f) -    %f      %f     %f     %f\n", elements, value0, value1 , value2, value3, value1/value3);

	*/

}

// Engloba el calculo para un nodo, paralelizable... aunque mejor empaquetar trabajos.
void GLWidget::computeNodeMasiveOptimized(DefGraph& graph, Modelo& model, int defId2)
{

	clock_t ini1, ini2, ini3, ini4, ini5; 
	clock_t fin1, fin2, fin3, fin4, fin5;

	long long mvc_mean = 0;
	long long arrange_mean = 0;
	long long precomputation_mean = 0;
	long long full_precomputation_mean = 0;
	long long buildMatrix_mean = 0;

	MatrixXf bigMatrix(model.vn(), model.vn());
	VectorXf bigVector(model.vn());

	bigMatrix.transposeInPlace();

	for(int i = 0; i< model.vn(); i++)
	{
		for(int j = i; j< model.vn(); j++)
		{
			float value = model.bind->BihDistances[0].get(i,j);
			bigMatrix(i,j) = value;
			bigMatrix(j,i) = value;
		}
	}


	float res1total, res2total;
	for(int defId = 0; defId < graph.deformers.size(); defId++ )
	{

		// calcular MVC
		ini1 = clock();
		mvcAllBindings(graph.deformers[defId]->pos, 
					   graph.deformers[defId]->MVCWeights, model);
		fin1 = clock();

		mvc_mean += (fin1-ini1);

		// Ordernar/cortar/Reordenar
		ini2 = clock();
		vector<ordWeight> weights(graph.deformers[defId]->MVCWeights.size());
		for(int i = 0; i< weights.size(); i++)
		{
			weights[i].idx = i;
			weights[i].weight = graph.deformers[defId]->MVCWeights[i];
		}

		VectorXf MVCWeights;
		VectorXi weightsSort;
		int simplifiedSize = getSignificantWeights(weights,
												   MVCWeights,
												   weightsSort);
		fin2 = clock();

		arrange_mean += (fin2-ini2);

		ini3 = clock();
		MatrixXf littleMatrix(weights.size(), simplifiedSize);
		for(int i = 0; i< simplifiedSize; i++)
		{
			int indI = weightsSort[i];
			littleMatrix.col(i) = bigMatrix.col(indI);
			/*
			for(int j = i+1; j< simplifiedSize; j++)
			{
				int indJ = weightsSort[j];
				float value = model.bind->BihDistances[0].get(indI,indJ);
				littleMatrix(j,i) = value;
				littleMatrix(i,j) = value;
			}

			littleMatrix(i,i) = model.bind->BihDistances[0].get(indI,indI);
			*/
		}
		fin3 = clock();

		buildMatrix_mean += (fin3-ini3);

		bigVector.fill(0);

		for(int i = 0; i<  simplifiedSize; i++)
		{
			bigVector[weightsSort[i]] = MVCWeights[i];
		}

		//VectorXf preubas(weights.size());
		// precalculo
		ini4 = clock();
		//preubas = MVCWeights*littleMatrix;
		
		/*for(int g = 0; g < weights.size(); g++)
		{
			//int indG = weightsSort[g];
			//preubas[g] = bigVector.transpose()*bigMatrix.col(g);//indG);
			preubas[g] = MVCWeights.dot(littleMatrix.col(g));//indG);
		}
		*/

		VectorXf preubas2(simplifiedSize);
		for(int c = 0; c < simplifiedSize; c++)
		{
			float tempValue = MVCWeights[c];
			for(int g = 0; g < simplifiedSize /*weights.size()*/; g++)
			{
				int indG = weightsSort[g];
				preubas2[g] += tempValue*littleMatrix(indG, c);
			}
		}
		
		
		//littleMatrix.transposeInPlace();
		
		//for(int g = 0; g < weights.size(); g++)
		//{
		//	preubas = MVCWeights*littleMatrix;
		//}
		

		//float res1 = preubas.dot(bigVector);
		float res1 = preubas2.dot(MVCWeights);
		fin4 = clock();

		//float res3 = MVCWeights.transpose()*littleMatrix*MVCWeights;

		precomputation_mean += (fin4-ini4);

		ini5 = clock();
		float res2 = bigVector.transpose()*bigMatrix*bigVector;
		fin5 = clock();

		if(defId == 0)
		{
			res1total = res1; 
			//res2total = res3;
		}

		full_precomputation_mean += (fin5-ini5);
	}

	// calculo de distancias

	int defNodesSize = graph.deformers.size();

	double temp1 = ((double)(mvc_mean))/CLOCKS_PER_SEC/defNodesSize;
	double temp2 = ((double)(arrange_mean))/CLOCKS_PER_SEC/defNodesSize;
	double temp3 = ((double)(precomputation_mean))/CLOCKS_PER_SEC/defNodesSize;
	double temp4 = ((double)(full_precomputation_mean))/CLOCKS_PER_SEC/defNodesSize;
	double temp5 = ((double)(buildMatrix_mean))/CLOCKS_PER_SEC/defNodesSize;

	// Tiempos
	printf("Dos valores de muestra %f %f... \n", res1total, res2total); 
	printf("\n\nValores de %d nodos en optimizado:\n ", defNodesSize);
	printf("Tiempo de mvc: %f\n", temp1);
	printf("Tiempo de ordenar: %f\n", temp2);
	printf("Tiempo de construccion matrix: %f\n", temp5);
	printf("Tiempo de precomputo: %f\n", temp3);
	printf("Tiempo de precomputo a full: %f\n\n\n",temp4);

}

void GLWidget::updateOutliner()
{
	AirRig* rig = (AirRig*) escena->rig;

	// Set Some parameters for better animation
	/*for(int i = 0; i< rig->defRig.defGroups.size(); i++)
	{
		int nodeIdAux = rig->defRig.defGroups[i]->nodeId;
		int trandsfromIdAux = rig->defRig.defGroups[i]->transformation->nodeId;
	}*/
	emit updateSceneView();
}

void GLWidget::enableDeformationAfterComputing()
{

	showInfo("Pesos computados...");

	AirRig* rig = (AirRig*) escena->rig;

	// TO REMOVE: This patch enables bulging just between the first and the second joint
	//for(int i = 0; i< rig->defRig.defGroups.size(); i++)
	//	rig->defRig.defGroups[1]->bulgeEffect = true;

	// Bulge initialization over skinning computation
	//initBulges(rig);

	// enable deformations and render data
	rig->enableDeformation = true;
	paintModelWithData();

	bDrawStatistics = true;

}


//////////////////////////////////////////////
//			ALL COMPUTATIONS          ////////
//////////////////////////////////////////////
void GLWidget::computeProcess() 
{
	bool localVerbose = true;

	clock_t ini, fin;

	// A. Volvemos a crear el rig de cero
	if(escena->rig) delete escena->rig;
	escena->rig = NULL;
	if(!escena->rig) escena->rig = new AirRig(scene::getNewId());
	AirRig* rig = (AirRig*) escena->rig;

	// B. Get values from UI
	float subdivisionRatio = parent->ui->bonesSubdivisionRatio->text().toFloat();

	//printf("Calculando con %f de factor de subdivision\n", subdivisionRatio);

	// C. Vincular a escena: modelo y esqueletos
	if(localVerbose) ini = clock();
	// C.1 - Bind the current skeleton
	rig->bindRigToModelandSkeleton((Modelo*)escena->models[0], escena->skeletons, subdivisionRatio);

	// C.2 - Build deformers structure for computations.
	BuildGroupTree(rig->defRig);
		
	if(localVerbose) fin = clock();

	if(localVerbose) 
	{
		printf("Preprocess: %f miliseconds\n", ((double)fin-ini)/CLOCKS_PER_SEC*1000); fflush(0);

		int defNodesCount = 0;
		for(int i = 0; i < rig->defRig.defGroups.size(); i++)
		{
			defNodesCount += rig->defRig.defGroups[i]->deformers.size();
		}

		printf("# de vertices: %d\n", rig->model->nodes.size());
		printf("# de huesos: %d\n", rig->defRig.defGroups.size());
		printf("Tenemos %d nodos (o %d)\n", defNodesCount, rig->defRig.deformers.size()); fflush(0);
	}

	// D. Launch the process

	// Disable deformations and updating
	for(int dgIdx = 0; dgIdx < rig->defRig.defGroups.size(); dgIdx++)
		rig->defRig.defGroups[dgIdx]->bulgeEffect = false;

	rig->enableDeformation = false;

	showInfo("Computing weights...");	

	if(compMgr.size() != rig->model->bind->surfaces.size() )
		compMgr.resize(rig->model->bind->surfaces.size());

	for(int idxSurf = 0; idxSurf < rig->model->bind->surfaces.size(); idxSurf++)
	{
		printf("Surface %d\n", idxSurf); fflush(0);

		compMgr[idxSurf].bd = rig->model->bind;
		compMgr[idxSurf].model = rig->model;
		compMgr[idxSurf].rig = rig;

		compMgr[idxSurf].surfaceIdx = idxSurf;

		// Do computations
		compMgr[idxSurf].updateAllComputations();
	}


	rig->defRig.cleanDefNodes();
	rig->cleanDefNodesDirtyBit();

	// Compute rest poses
	rig->saveRestPoses();

	// Enable deformations and updating
	enableDeformationAfterComputing();
	updateOutliner();

	showInfo("Weights computed...");

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

			group->dirtySmooth = true;

			/*
			AirRig* rig = (AirRig*)escena->rig;
			updateAirSkinning(rig->defRig, *rig->model);
			*/
			//appMgr->start();

			AirRig* localRig = (AirRig*)escena->rig;

			for(int dgIdx = 0; dgIdx < localRig->defRig.defGroups.size(); dgIdx++)
				localRig->defRig.defGroups[dgIdx]->bulgeEffect = false;

			localRig->enableDeformation = false;

			showInfo("Updating weights...");	

			// Do computations
			for(int surfIdx = 0; surfIdx< compMgr.size(); surfIdx++)
			{
				compMgr[surfIdx].updateAllComputations();
			}

			localRig->defRig.cleanDefNodes();

			// Enable deformations and updating
			enableDeformationAfterComputing();

			showInfo("Weights updated...");	
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

			rig->defRig.defGroups[defIdx]->dirtySmooth = true;
		}

		for(int dgIdx = 0; dgIdx < rig->defRig.defGroups.size(); dgIdx++)
				rig->defRig.defGroups[dgIdx]->bulgeEffect = false;

		rig->enableDeformation = false;

		showInfo("Updating weights...");	

		// Do computations
		for (int surfIdx = 0; surfIdx < rig->model->bind->surfaces.size(); surfIdx++)
		{
			compMgr[surfIdx].updateAllComputations();
		}

		rig->defRig.cleanDefNodes();

		// Enable deformations and updating
		enableDeformationAfterComputing();

		showInfo("Weights updated...");	

		//appMgr->start();
		//updateAirSkinning(rig->defRig, *rig->model);
	}

	paintModelWithData();
}

void GLWidget::enableRTInteraction(bool enable)
{
	m_bRTInteraction = enable;
}

void GLWidget::setBulgeParams(bool enable)
{
	AirRig* rig = (AirRig*)escena->rig;
	if(rig && rig->airSkin)
	{
		for(int bulgeIdx = 0; bulgeIdx < rig->airSkin->bulge.size(); bulgeIdx++)
			rig->airSkin->bulge[bulgeIdx].enabled = enable;
	}
}

void GLWidget::setTwistParams(double ini, double fin, bool enable, bool smooth)
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
			group->smoothTwist = smooth;
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

	map<int, joint*> savedPoses;

	if (selectedObject != NULL) 
	{
		if(selectedObject->iam == DEFGROUP_NODE)
		{
			AirRig* currentRig = ((AirRig*)sktCr->parentRig);

			DefGroup* group = (DefGroup*) selectedObject;
			group->expansion = expValue;

			propagateExpansion(*group);
			
			AirRig* rig = (AirRig*)escena->rig;

			if(!rig)
			{
				printf("The Rig is not initilized... do something, please!\n");
			}

			// Disable deformations and updating
			for(int dgIdx = 0; dgIdx < rig->defRig.defGroups.size(); dgIdx++)
				rig->defRig.defGroups[dgIdx]->bulgeEffect = false;

			rig->enableDeformation = false;
			assert(compMgr.size() > 0);

			// Do computations
			for(int surfIdx = 0; surfIdx < rig->model->bind->surfaces.size(); surfIdx++)
			{
				compMgr[surfIdx].updateAllComputations();
			}

			rig->defRig.cleanDefNodes();

			// Enable deformations and updating
			enableDeformationAfterComputing();
		}
	}
 }

//void GLWidget::postSelection(const QPoint&)
//{
//    int i = 0;
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
//}

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

void GLWidget::drawBulgeCurve(int idDeformer, int idGroup, Vector2i _Origin,Vector2i rectSize)
{

	// Backgound
	glColor4f(0.35f, 0.35f, 0.35f, 0.2);
	glBegin(GL_QUADS);
		glVertex2f(_Origin.x(), 0.0);
		glVertex2f(_Origin.x()+ rectSize.x(), 0.0);
		glVertex2f(_Origin.x()+ rectSize.x(), rectSize.y());
		glVertex2f(_Origin.x(), rectSize.y());
	glEnd();

	// Border
	glLineWidth(3);
	glColor3f(0.85f, 0.85f, 0.85f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(_Origin.x(), _Origin.y() + 0.0);
		glVertex2f(_Origin.x()+ rectSize.x(), _Origin.y() + 0.0);
		glVertex2f(_Origin.x()+ rectSize.x(), _Origin.y() + rectSize.y());
		glVertex2f(_Origin.x()+ 0.0, _Origin.y() + rectSize.y());
	glEnd();

	int margen = 15;
	Vector2i origin(_Origin.x()+margen,_Origin.y()+margen);
	Vector2i size = rectSize - Vector2i(margen,margen)*2;

	float miniLine = 5;

	// Axis
	glLineWidth(2);
	glColor3f(0.85f, 0.85f, 0.85f);
	glBegin(GL_LINES);
		// BASE
		glVertex2f(origin.x()-miniLine, origin.y());
		glVertex2f(origin.x()+ size.x(), origin.y());
		glVertex2f(origin.x(), origin.y()-miniLine);
		glVertex2f(origin.x(), origin.y()+ size.y());

		//TICS
		glVertex2f(origin.x() - miniLine, origin.y()+ size.y());
		glVertex2f(origin.x() + miniLine, origin.y()+ size.y());
		glVertex2f(origin.x()+ size.x(), origin.y() - miniLine);
		glVertex2f(origin.x()+ size.x(), origin.y() + miniLine);

		glVertex2f(origin.x()+size.x()/2, origin.y()-miniLine);
		glVertex2f(origin.x()+size.x()/2, origin.y()+miniLine);
		glVertex2f(origin.x()-miniLine , origin.y() + size.y()/2);
		glVertex2f(origin.x()+miniLine, origin.y() + size.y()/2);

	glEnd();

	double maxX = 1.0;
	double maxY = 1.0;

	vector<Vector2f> cpoints;
	//cpoints.push_back(Vector2f(0.0,0.0));
	//cpoints.push_back(Vector2f(40.0,0.0));
	//cpoints.push_back(Vector2f(60.0,0.2));
	//cpoints.push_back(Vector2f(63.0,0.05));
	//cpoints.push_back(Vector2f(80.0,0.0));
	//cpoints.push_back(Vector2f(82.0,0.25));
	//cpoints.push_back(Vector2f(90,0.0));

	/*
	cpoints.push_back(Vector2f(0.0,0.0));
	cpoints.push_back(Vector2f(0.2,0.5));
	cpoints.push_back(Vector2f(0.9,0.5));
	cpoints.push_back(Vector2f(0.95,0.5));
	cpoints.push_back(Vector2f(1.0,0.5));
	*/

	// wrinkle controled
	cpoints.push_back(Vector2f(0.0,0.0));
	//cpoints.push_back(Vector2f(0.01,0.01));
	cpoints.push_back(Vector2f(0.5,0.5));
	//cpoints.push_back(Vector2f(0.99,0.01));
	cpoints.push_back(Vector2f(1.0,0.0));
	
	// Creamos una curva
	AirSkinning* skin = ((AirRig*)escena->rig)->airSkin;

	if(skin->bulge.size() <= idDeformer) return;

	//   Bulge effect inicialization
	if(skin->bulge[idDeformer].groups.size() > idGroup)
	{
		// Cogemos una de las curvas para hacer pruebas
		Spline& spline = skin->bulge[idDeformer].groups[idGroup]->defCurve->W;

		bool noData = false;
		if(spline.size() == 0)
		{
			spline.addPoint(0.0, 0.0);
			spline.addPoint(0.5, 0.0);
			spline.addPoint(1.0, 0.0);
			noData = true;
		}
		//Using the specified points
		//for(int i = 0; i< cpoints.size(); i++)
		//{
		//	spline.addPoint(cpoints[i].x()/maxX, cpoints[i].y()/maxY);
		//}

		spline.setLowBC(Spline::FIXED_2ND_DERIV_BC, 0);
		spline.setHighBC(Spline::FIXED_2ND_DERIV_BC, 0);

		glPointSize(6);
		glColor3f(0.9f, 0.1f, 0.1f);

		glBegin(GL_POINTS);
		Spline::const_iterator it = spline.begin();
		for(; it != spline.end(); it++)
		{
			double x =  it->first;
			double y = it->second;
			glVertex2f(origin.x()+x/maxX*size.x(), origin.y()+ y/maxY*size.y());
		}
		glEnd();

		glLineWidth(1);
		if(noData)
			glColor3f(0.9f, 0.1f, 0.1f);
		else
			glColor3f(0.9f, 0.9f, 0.9f);
		glBegin(GL_LINES);

		double preX = 0.0;
		double preY = spline(preX);
		for (double x = 0.005 ; x <= 1.0; x += 0.005)
		{
			glVertex2f(preX*size.x()+ origin.x(), preY*size.y()+ origin.y());
			glVertex2f(x*size.x()+ origin.x(), spline(x)*size.y()+ origin.y());
			preX = x; preY = spline(x);
		}

		if(noData)
			spline.clear();

		glEnd();
	}
}

void GLWidget::drawBulgePressurePlane(int idDeformer, int idGroup, Vector2i _Origin,Vector2i rectSize)
{
	// Backgound
	glColor4f(0.35f, 0.35f, 0.35f, 0.2);
	glBegin(GL_QUADS);
		glVertex2f(_Origin.x(), _Origin.y());
		glVertex2f(_Origin.x()+ rectSize.x(), _Origin.y());
		glVertex2f(_Origin.x()+ rectSize.x(), _Origin.y()+rectSize.y());
		glVertex2f(_Origin.x(), _Origin.y()+rectSize.y());
	glEnd();

	// Border
	glLineWidth(3);
	glColor3f(0.85f, 0.85f, 0.85f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(_Origin.x(), _Origin.y() + 0.0);
		glVertex2f(_Origin.x()+ rectSize.x(), _Origin.y() + 0.0);
		glVertex2f(_Origin.x()+ rectSize.x(), _Origin.y() + rectSize.y());
		glVertex2f(_Origin.x()+ 0.0, _Origin.y() + rectSize.y());
	glEnd();

	int margen = 15;
	Vector2i origin(_Origin.x()+margen,_Origin.y()+margen);
	Vector2i size = rectSize - Vector2i(margen,margen)*2;

	float miniLine = 5;

	// Axis
	glLineWidth(2);
	glColor3f(0.85f, 0.85f, 0.85f);
	glBegin(GL_LINES);
		// BASE
		glVertex2f(origin.x()-miniLine, origin.y());
		glVertex2f(origin.x()+ size.x(), origin.y());
		glVertex2f(origin.x(), origin.y()-miniLine);
		glVertex2f(origin.x(), origin.y()+ size.y());
		glVertex2f(origin.x()+ size.x(), origin.y()-miniLine);
		glVertex2f(origin.x()+ size.x(), origin.y()+ size.y());

		//TICS
		glVertex2f(origin.x() - miniLine, origin.y()+ size.y());
		glVertex2f(origin.x() + miniLine, origin.y()+ size.y());
		glVertex2f(origin.x()+ size.x(), origin.y() - miniLine);
		glVertex2f(origin.x()+ size.x(), origin.y() + miniLine);

		glVertex2f(origin.x()+size.x()/2, origin.y()-miniLine);
		glVertex2f(origin.x()+size.x()/2, origin.y()+miniLine);
		glVertex2f(origin.x()-miniLine , origin.y() + size.y()/2);
		glVertex2f(origin.x()+miniLine, origin.y() + size.y()/2);

	glEnd();
	
	// Creamos una curva
	AirSkinning* skin = ((AirRig*)escena->rig)->airSkin;

	if(skin->bulge.size() <= idDeformer) return;

	//   Bulge effect inicialization
	if(skin->bulge[idDeformer].groups.size() > idGroup)
	{

		float maxX = skin->bulge[idDeformer].groups[idGroup]->maxI[0]-skin->bulge[idDeformer].groups[idGroup]->minI[0];
		float maxY = skin->bulge[idDeformer].groups[idGroup]->maxI[1]-skin->bulge[idDeformer].groups[idGroup]->minI[1];

		float minX = skin->bulge[idDeformer].groups[idGroup]->minI[0];
		float minY = skin->bulge[idDeformer].groups[idGroup]->minI[1];

		for(int pptIdx = 0; pptIdx < skin->bulge[idDeformer].groups[idGroup]->pressurePoints.size(); pptIdx++)
		{
			Vector2f& ppt = skin->bulge[idDeformer].groups[idGroup]->pressurePoints[pptIdx];
			float press = skin->bulge[idDeformer].groups[idGroup]->pressure[pptIdx];

			glPointSize(8*press);

			float r,g,b;
			GetColour(press/5, 0, 1, r,g,b);

			glColor3f(r, g, b);
			glBegin(GL_POINTS);
			glVertex2f(origin.x() + size.x()/2 +ppt.x()*20, origin.y() + ppt.y()*20);
			glEnd();
		}
	}
}

void GLWidget::draw2DGraphics()
{
	int groupIdx = 0;
	if(selMgr.selection.size() == 1)
	{
		if(selMgr.selection[0]->iam == DEFGROUP_NODE)
		{
			int foundIdx = -1;
			AirSkinning* skin = ((AirRig*)escena->rig)->airSkin;
			for(int bulgeIdx = 0; bulgeIdx< skin->bulge.size(); bulgeIdx++)
			{
				if(skin->bulge[bulgeIdx].groups[0]->childDeformerId == selMgr.selection[0]->nodeId)
				{
					foundIdx = bulgeIdx;
					break;
				}
			}

			if(foundIdx >= 0)
			{
				BulgeGroup* def = skin->bulge[foundIdx].groups[groupIdx];
				wireDeformer* curve = def->defCurve;
				glPointSize(10);
				glColor3f(1.0,0,0);
				glBegin(GL_POINTS);
				glVertex3f(curve->pos.x(), curve->pos.y(), curve->pos.z());
				glColor3f(0.0,1.0,1.0);
				glVertex3f(curve->origin.x(), curve->origin.y(), curve->origin.z());
				glColor3f(1.0,0,1.0);
				glVertex3f(curve->D.x(), curve->D.y(), curve->D.z());
				glEnd();

				glPointSize(7);
				glBegin(GL_POINTS);
				for(int ptIdx = 0; ptIdx < skin->bulge[foundIdx].groups[groupIdx]->refPoints.size(); ptIdx++)
				{
					glColor3f(1.0,(float)ptIdx/(float)skin->bulge[foundIdx].groups[groupIdx]->refPoints.size(),1.0);
					Vector3d pt = skin->bulge[foundIdx].groups[groupIdx]->refPoints[ptIdx];
					glVertex3f(pt.x(), pt.y(), pt.z());
				}
				glEnd();

				float length = curve->lenght;
				float height = curve->high;

				Vector3d rad = curve->mDir.cross(curve->mN)*curve->r;

				glLineWidth(2);
				glColor3f(1.0,0,0);
				glBegin(GL_LINES);
				glVertex3f(curve->pos.x(), curve->pos.y(), curve->pos.z());
				glVertex3f(curve->pos.x()+curve->mDir.x()*length, 
						   curve->pos.y()+curve->mDir.y()*length, 
						   curve->pos.z()+curve->mDir.z()*length);

				glColor3f(1.0,1.0,0);
				glVertex3f(curve->pos.x(), curve->pos.y(), curve->pos.z());
				glVertex3f(curve->pos.x()+curve->mN.x()*height, 
						   curve->pos.y()+curve->mN.y()*height, 
						   curve->pos.z()+curve->mN.z()*height);

				glColor3f(0.0,1.0,1.0);
				glVertex3f(curve->pos.x(), curve->pos.y(), curve->pos.z());
				glVertex3f(curve->pos.x()+rad.x(), 
						   curve->pos.y()+rad.y(), 
						   curve->pos.z()+rad.z());
				glEnd();

				// Pintamos la curva
				glLineWidth(2);
				glColor3f(1.0,1.0,1.0);
				glBegin(GL_LINES);
				
				Vector3d orig = curve->pos;
				Vector3d dirX = curve->mDir;
				Vector3d dirY = curve->mN;
				for(int cuIdx = 1; cuIdx < 1000; cuIdx++)
				{
					float posX = (cuIdx-1)/1000.0;
					float posX2 = cuIdx/1000.0;
					if(curve->W.size() == 0) continue;
					
					Vector3d pt1 = orig+posX*length*dirX+curve->W(posX)*height*dirY;
					Vector3d pt2 = orig+posX2*length*dirX+curve->W(posX2)*height*dirY;
					
					glVertex3f(pt1.x(), pt1.y(), pt1.z());
					glVertex3f(pt2.x(), pt2.y(), pt2.z());

				}
				glEnd();

				glColor3f(0.8,0.2,0.2);
				glBegin(GL_LINES);
				
				for(int cuIdx = 1; cuIdx < 1000; cuIdx++)
				{
					float posX = (cuIdx-1)/1000.0;
					float posX2 = cuIdx/1000.0;
					if(curve->R.size() == 0) continue;
					
					Vector3d pt1 = orig+posX*length*dirX+curve->R(posX)*height*dirY;
					Vector3d pt2 = orig+posX2*length*dirX+curve->R(posX2)*height*dirY;
					
					glVertex3f(pt1.x(), pt1.y(), pt1.z());
					glVertex3f(pt2.x(), pt2.y(), pt2.z());

				}
				glEnd();

				glPointSize(10);
				glBegin(GL_POINTS);
				for(int gvIdx = 0; gvIdx < skin->bulge[foundIdx].groups[groupIdx]->vertexGroup.size(); gvIdx++)
				{
					int nodeIdx = skin->bulge[foundIdx].groups[groupIdx]->vertexGroup[gvIdx];
					Vector3d pointForPaint = skin->deformedModel->nodes[nodeIdx]->position;

					float r,g,b;
					GetColour(skin->bulge[foundIdx].groups[groupIdx]->w[gvIdx], 0, 1, r, g, b);

					glColor3f(r,g,b);
					glVertex3f(pointForPaint.x(), pointForPaint.y(), pointForPaint.z());
				}
				glEnd();
			}
		}
	}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glOrtho(0, width(), 0, height(), 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	int rectWidth = 250;
	int rectheight = 180;

	Vector2i rectSize(rectWidth, rectheight);
	Vector2i squareSize(rectheight, rectheight);

	if(selMgr.selection.size() == 1)
	{
		if(selMgr.selection[0]->iam == DEFGROUP_NODE)
		{
			int foundIdx = -1;
			AirSkinning* skin = ((AirRig*)escena->rig)->airSkin;
			for(int bulgeIdx = 0; bulgeIdx< skin->bulge.size(); bulgeIdx++)
			{
				if(skin->bulge[bulgeIdx].groups[0]->childDeformerId == selMgr.selection[0]->nodeId)
				{
					foundIdx = bulgeIdx;
					break;
				}
			}

			if(foundIdx >= 0)
			{
				drawBulgeCurve(foundIdx, 1, Vector2i(0,0), rectSize);
				drawBulgePressurePlane(foundIdx, 1, Vector2i(rectWidth + 20,0), squareSize);
			}
		}
	}

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

}

void GLWidget::drawFPS(float fps, int x, int y)
{
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width(), 0, height(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}

 void GLWidget::draw()
 {
	 clock_t time = clock();
	 clock_t interval = time - last_time;
	 last_time = time;
	 float fps = 1.0/ ((float)interval / (float)CLOCKS_PER_SEC);

	 //setFPSIsDisplayed(true);
	 //printf("FPS: %f\n", fps);

	 //drawFPS(fps, 15, height() - 15);

	// Clear the color buffer
	glClear( GL_COLOR_BUFFER_BIT );

	// Set the shader as current
	setShaderConfiguration(preferredType);

	clock_t ini = clock();
	clock_t fin;

	for(int j = 0; j < escena->skeletons.size(); j++) 
	{
		escena->skeletons[j]->root->computeWorldPos();
	}

	// Draw analisis structures
	if (m_bShowAnalisis)
	{
		pushShader(SHD_NO_SHADE);
		for (unsigned int i = 0; i < escena->models.size(); i++)
		{
			((Modelo*)escena->models[i])->drawAnalitics();	
		}
		popShader();
	}


	AirRig* rig = (AirRig*)escena->rig;

	if(rig) 
	{
		for(int j = 0; j < rig->defRig.roots.size(); j++) 
		{
			//rig->defRig.roots[j]->computeWorldPosNonRoll(rig->defRig.roots[j]);
			rig->defRig.roots[j]->computeWorldPos(rig->defRig.roots[j]);
		}

		if(rig->enableDeformation)
		{
			//if(!rig->enableTwist)
				rig->airSkin->computeDeformations(rig);
			//else
			//	rig->airSkin->computeDeformationsWithSW(rig);
		}
	}

	setShaderConfiguration(preferredType);
	AdriViewer::draw();

	pushShader(SHD_NO_SHADE);

	glDisable(GL_DEPTH_TEST);

	// Dynamic creative rigg
	if(sktCr && sktCr->dynRig && AirRig::mode == MODE_CREATE) 
	{
		sktCr->dynRig->update();
		for(unsigned int i = 0; i< sktCr->dynRig->defRig.defGroups.size(); i++)
		{
			((DefGroupRender*)sktCr->dynRig->defRig.defGroups[i]->shading)->drawFunc();
		}
	}
	

	// AirRig de la escena
	for(unsigned int i = 0; i< rig->defRig.defGroups.size(); i++)
    {
        ((DefGroupRender*)rig->defRig.defGroups[i]->shading)->drawFunc();
    }

	// Manipuladores
	if(selMgr.selection.size() >0 )
	{
		ToolManip.drawFunc();
	}

	popShader();
	glEnable(GL_DEPTH_TEST);

	if(showBulgeInfo)
		draw2DGraphics();

 }

void GLWidget::drawWithNames()
{
	if(selMgr.currentTool == CTX_CREATE_SKT)
	{
		// Enable selection with defGroups
		// from "all the rigs" in the scene
		AirRig* rig = (AirRig*)escena->rig;
		for(int rigIdx = 0; rigIdx < rig->defRig.defGroups.size(); rigIdx++)
		{
			glPushName(rig->defRig.defGroups[rigIdx]->nodeId);
			((DefGroupRender*)rig->defRig.defGroups[rigIdx]->shading)->drawWithNames();
			glPopName();
		}
	}
	else
	{
		AdriViewer::drawWithNames();
	}
}

void GLWidget::finishRiggingTool()
{
	// disable deormation until the weights are computed
	((AirRig*)escena->rig)->enableDeformation = false;

	//printf("Finalizando operacion\n");
	sktCr->finishRig();

	// Deseleccionar todo.
	for(int i = 0; i< selMgr.selection.size(); i++)
	{
		selMgr.selection[i]->select(false, selMgr.selection[i]->nodeId);
	}

	selMgr.selection.clear();
	sktCr->state = SKT_CR_IDDLE;
	
	emit setRiggingToolUI();
	sktCr->mode = SKT_RIGG;

	for (int rootIdx = 0; rootIdx < sktCr->parentRig->defRig.roots.size(); rootIdx++)
	{
		sktCr->parentRig->defRig.roots[rootIdx]->computeWorldPos(sktCr->parentRig->defRig.roots[rootIdx]);
	}

	float subdivsionRatio = parent->ui->bonesSubdivisionRatio->text().toFloat();
	for (int dgIdx = 0; dgIdx < sktCr->parentRig->defRig.defGroups.size(); dgIdx++)
	{
		sktCr->parentRig->defRig.defGroups[dgIdx]->subdivisionRatio = subdivsionRatio;
	}

	// Solo se actualizará el que este marcado.
	for (int dgIdx = 0; dgIdx < sktCr->parentRig->defRig.defGroups.size(); dgIdx++)
	{
		sktCr->parentRig->defRig.defGroups[dgIdx]->update();
	}

	sktCr->parentRig->update();

	//1.Process rig->anim mode
	sktCr->parentRig->saveRestPoses();

	// Compute the new rigg
	computeWeights();

	// Paint the model acordingly
	paintModelWithData();

	preferredNormalType = SHD_VERTEX_COLORS;
	emit updateSceneView();
}

void GLWidget::keyReleaseEvent(QKeyEvent* e)
{
	bool handled = false;
	
	if(e->key() == Qt::Key_X)
	{
		X_ALT_modifier = false;
		handled = true;
	}
	else if(e->key() == Qt::Key_Alt)
	{
		X_ALT_modifier = false;
		handled = true;
	}

	if (!handled)
		QGLViewer::keyReleaseEvent(e);
}

void GLWidget::keyPressEvent(QKeyEvent* e)
{
 	bool handled = false;
	//const Qt::ButtonState modifiers = (Qt::ButtonState)(e->state() & Qt::KeyButtonMask);
	if(e->key() == Qt::Key_Return)
	{
		if(sktCr->state == SKT_CR_SELECTED)
		{
			finishRiggingTool();
			handled = true;
		}
	}
	else if(e->key() == Qt::Key_Backspace)
	{
		//TODO
		assert(false);
		handled = true;
	}
	else if(e->key() == Qt::Key_X)
	{
		// Como si fuera el alt
		X_ALT_modifier = true;
		handled = true;
	}
	else if(e->key() == Qt::Key_C)
	{
		// Activate create skt tool
		handled = true;
	}
	else if(e->key() == Qt::Key_V)
	{
		// Activate anim skt tool
		handled = true;
	}
	else if(e->key() == Qt::Key_B)
	{
		// Activate test skt tool
		handled = true;
	}
	else if(e->key() == Qt::Key_Alt)
	{
		X_ALT_modifier = true;
		handled = true;
	}	

	else if (e->key() == Qt::Key_Left ||
		e->key() == Qt::Key_Right||
		e->key() == Qt::Key_Up ||
		e->key() == Qt::Key_Down)
	{
		moveThroughHierarchy(e->key());
		handled = true;
	}
	else if (e->key() == Qt::Key_F)
	{
		if (selMgr.selection.size() > 0)
		{
			if (selMgr.selection[0]->iam == DEFGROUP_NODE)
			{
				DefGroup* dg = (DefGroup*)selMgr.selection[0];
				setSceneCenter(qglviewer::Vec(dg->transformation->worldPosition.x(), 
					dg->transformation->worldPosition.y(),
					dg->transformation->worldPosition.z()));

				Vector3d minPt, maxPt;
				dg->getBoundingBox(minPt, maxPt);

				float diagonal = (maxPt - minPt).norm();

				if (diagonal < 0.01) diagonal = 0.01;

				setSceneRadius(diagonal);

				// Hay que mover la camara
				showEntireScene();
			}
		}

		handled = true;
	}


	if (!handled)
		QGLViewer::keyPressEvent(e);
}


int GLWidget::getSelection()
{
  // Flush GL buffers
  glFlush();

  // Get the number of objects that were seen through the pick matrix frustum. Reset GL_RENDER mode.
  GLint nbHits = glRenderMode(GL_RENDER);

  // Each hit has 4 values...
  if (nbHits > 0)
  {
	if(debugingSelection)
	{
		printf("Number of hits: %d\n", nbHits);
		for(int i = 0; i< nbHits; i++)
		{
			printf("Elements: %d %d %d %d \n", (selectBuffer())[4*i+0], (selectBuffer())[4*i+1], 
											   (selectBuffer())[4*i+2], (selectBuffer())[4*i+3]);
		}
	}

	// We get the first node...
	int firstId = (selectBuffer())[4*0+3];
	return firstId;
  }
  else if(debugingSelection)
	printf("No selection\n");

  return  -1;
}

void GLWidget::mousePressEvent(QMouseEvent* e)
{
	dragged = false;

	if(e->button() == Qt::LeftButton && !X_ALT_modifier)
	{
		//printf("pressed Left Button\n");
		pressMouse = Vector2f(e->pos().x(), e->pos().y());
		pressed = true;
		dragged = false;

		// Just init the manipulator to work properly
		if(selMgr.toolCtx == CTX_MOVE || 
		   selMgr.toolCtx == CTX_ROTATION || 
		   selMgr.toolCtx == CTX_SCALE)
		{
			beginSelection(e->pos());
			ToolManip.drawFuncNames();
			int node = getSelection();

			if(node >= 0 && node <10)
			{
				//printf("AxisNode selected\n");
				ToolManip.bModifierMode = true;

				qglviewer::Vec orig, dir, selectedPoint;
				camera()->convertClickToLine(QPoint(e->pos().x(), e->pos().y()), orig, dir);

				if(node == 0) ToolManip.axis = AXIS_VIEW;
				if(node == 1) ToolManip.axis = AXIS_X;
				if(node == 2) ToolManip.axis = AXIS_Y;
				if(node == 3) ToolManip.axis = AXIS_Z;

				qglviewer::Vec camPos = camera()->position();
				ToolManip.cameraPos = Vector3d(camPos.x, camPos.y, camPos.z);

				Vector3d rayOrigin(orig.x, orig.y, orig.z);
				Vector3d rayDir(dir.x, dir.y, dir.z);
				ToolManip.startManipulator(rayOrigin, rayDir);

				//Ver si es un manipulador.
				//printf("Es el elemento %d del manipulador\n", node);
			}
		}
	}

	AdriViewer::mousePressEvent(e);
}


void GLWidget::resetAnimation()
{
	AirRig* currentRig = (AirRig*)escena->rig;
	((AirRig*)escena->rig)->restorePoses();

	// No esta inicializado todavia.
	if (!currentRig->airSkin) return;

	currentRig->airSkin->resetDeformations();
}

void GLWidget::moveThroughHierarchy(int keyCode)
{
	if (selMgr.selection.size() == 0) return;

	if (selMgr.selection[0]->iam != DEFGROUP_NODE) return;

	AirRig* currentRig = ((AirRig*)sktCr->parentRig);
	object* obj = selMgr.selection[0];
	DefGroup* dg = (DefGroup*)obj;

	DefGroup* newSel = (DefGroup*)obj;

	if (keyCode == Qt::Key_Up)
	{
		if (dg->dependentGroups.size() > 0)
			newSel = dg->dependentGroups[0];
		else
			newSel = dg;
	}
	else if (keyCode == Qt::Key_Down)
	{
		if (dg->relatedGroups.size() > 0)
			newSel = dg->relatedGroups[0];
		else
			newSel = dg;
	}
	else if (keyCode == Qt::Key_Left)
	{
		if (dg->dependentGroups.size() > 0)
		{
			DefGroup* dgFather = dg->dependentGroups[0];

			int groupId = -1;
			for (int i = 0; i < dgFather->relatedGroups.size(); i++)
			{
				if (dgFather->relatedGroups[i]->nodeId == dg->nodeId)
				{
					groupId = i;
					break;
				}
			}

			if (groupId == -1)
			{
				newSel = dg;
			}
			else
			{
				newSel = dgFather->relatedGroups[(groupId + 1) % dgFather->relatedGroups.size()];
			}

		}
		else
			newSel = dg;
	}
	else if (keyCode == Qt::Key_Right)
	{
		if (dg->dependentGroups.size() > 0)
		{
			DefGroup* dgFather = dg->dependentGroups[0];

			int groupId = -1;
			for (int i = 0; i < dgFather->relatedGroups.size(); i++)
			{
				if (dgFather->relatedGroups[i]->nodeId == dg->nodeId)
				{
					groupId = i;
					break;
				}
			}

			if (groupId == -1)
			{
				newSel = dg;
			}
			else
			{
				if (groupId == 0)
					newSel = dgFather->relatedGroups.back();
				else
					newSel = dgFather->relatedGroups[groupId - 1];
			}

		}
		else
			newSel = dg;
	}


	if (newSel->nodeId != dg->nodeId)
	{
		// deseleccion
		int oldId = dg->nodeId;
		dg->select(false, oldId);
		selMgr.selection.clear();

		vector<unsigned int > lst;
		lst.push_back(newSel->nodeId);
		selectElements(lst);

		// Selection
		selMgr.selection.push_back(newSel);
		newSel->select(true, newSel->nodeId);
		
		// Actualizar el manipulador
		ToolManip.bModifierMode = false;
		ToolManip.bEnable = true;
		ToolManip.setFrame(newSel->transformation->translation, newSel->transformation->rotation, Vector3d(1, 1, 1));

	}


}

void GLWidget::mouseMoveEvent(QMouseEvent* e)
{
	if(!X_ALT_modifier)
	{
		if(pressed && ToolManip.bModifierMode)
		{
			AirRig* currentRig = ((AirRig*)sktCr->parentRig);

			if (selMgr.selection.size() == 0) return;

			object* obj = selMgr.selection[0];
			DefGroup* dg = (DefGroup*)obj;

			Vector2i currentMouse(e->pos().x(), e->pos().y());

			qglviewer::Vec orig, dir, selectedPoint;
			camera()->convertClickToLine(QPoint(e->pos().x(), e->pos().y()), orig, dir);

			// No movement
			if(e->pos().x() == ToolManip.pressMouse.x() && e->pos().y() == ToolManip.pressMouse.y() )
				return;

			// Transform manipulator
			Vector3d rayOrigin(orig.x, orig.y, orig.z);
			Vector3d rayDir(dir.x, dir.y, dir.z);
			ToolManip.moveManipulator(rayOrigin, rayDir);
			
			if(currentRig && (sktCr->mode == SKT_CREATE || sktCr->mode == SKT_RIGG))
			{
				// Transform the object
				ToolManip.applyTransformation(selMgr.selection[0], ToolManip.type, ToolManip.mode);

				sktCr->parentRig->dirtyFlag = true;
				sktCr->parentRig->update();

				//1.Process rig->anim mode
				currentRig->saveRestPoses();

				if (m_bRTInteraction)
					computeWeights();

				paintModelWithData();
			}
			else if (currentRig && (sktCr->mode == SKT_TEST))
			{

				// 1. Compute the displacement in local
				Vector3d newPosition(0, 0, 0);
				Vector3d displ = ToolManip.currentframe.position;

				Quaterniond currentRot = Quaterniond::Identity();
				Quaterniond currentrRot = Quaterniond::Identity();

				Vector3d currentPos(0, 0, 0);
				Vector3d currentRestPos(0, 0, 0);

				if (dg->dependentGroups.size() > 0)
				{
					currentRot = dg->dependentGroups[0]->transformation->rotation;
					currentrRot = dg->dependentGroups[0]->transformation->rRotation;

					currentPos = dg->dependentGroups[0]->transformation->translation;
					currentRestPos = dg->dependentGroups[0]->transformation->rTranslation;
				}

				displ = currentRot.inverse()._transformVector(displ - currentPos); // Transformacion a local
				displ = currentrRot._transformVector(displ) + currentRestPos; // Transformacion a local		

				map<int, joint*> pose;
				for (int jointIdx = 0; jointIdx < currentRig->defRig.defGroups.size(); jointIdx++)
				{
					pose[currentRig->defRig.defGroups[jointIdx]->nodeId] = new joint();
					pose[currentRig->defRig.defGroups[jointIdx]->nodeId]->copyBasicData(currentRig->defRig.defGroups[jointIdx]->transformation);
				}

				// 3. Restore the pose
				currentRig->restorePoses();

				// FOR TESTING REASONS
				if (m_bShowAnalisis)
				{
					ToolManip.jointsPoints.clear();
					ToolManip.jointsColors.clear();
					for (int defGroupAIdx = 0; defGroupAIdx < currentRig->defRig.defGroups.size(); defGroupAIdx++)
					{
						//ToolManip.jointsPoints.push_back(currentRig->defRig.defGroups[defGroupAIdx]->transformation->translation);
						DefGroup* dgAux = currentRig->defRig.defGroups[defGroupAIdx];
						for (int defNodeAIdx = 0; defNodeAIdx < dgAux->deformers.size(); defNodeAIdx++)
						{
							if (!dgAux->deformers[defNodeAIdx].freeNode)
							{
								ToolManip.jointsPoints.push_back(dgAux->deformers[defNodeAIdx].pos);

								Vector3i nodeCell = currentRig->model->grid->cellId(dgAux->deformers[defNodeAIdx].pos);

								bool in = (nodeCell.x() >= 0 && nodeCell.y() >= 0 && nodeCell.z() >= 0 &&
									nodeCell.x() < currentRig->model->grid->dimensions.x() &&
									nodeCell.y() < currentRig->model->grid->dimensions.y() &&
									nodeCell.z() < currentRig->model->grid->dimensions.z());

								if (in && currentRig->model->grid->isContained(nodeCell, 0))
								{
									ToolManip.jointsColors.push_back(Vector3d(0, 1, 0));
								}
								else
								{
									ToolManip.jointsColors.push_back(Vector3d(1, 0, 0));
								}
							}
						}

						ToolManip.jointsPoints.push_back(dgAux->transformation->rTranslation);
						ToolManip.jointsColors.push_back(Vector3d(0, 0, 1));
						ToolManip.jointsPoints.push_back(dgAux->transformation->translation);
						ToolManip.jointsColors.push_back(Vector3d(0.5, 0.5, 0.9));
					}
				}


				dg->setTranslation(displ.x(), displ.y(), displ.z(), ToolManip.mode == TM_SINGLE);

				dg->dirtyFlag = true; 
				sktCr->parentRig->dirtyFlag = true;
				sktCr->parentRig->update();

				for (int rootIdx = 0; rootIdx < sktCr->parentRig->defRig.roots.size(); rootIdx++)
				{
					sktCr->parentRig->defRig.roots[rootIdx]->computeWorldPos(sktCr->parentRig->defRig.roots[rootIdx]);
				}

				dg->update();

				// Pose the model in the original situation, that could be voided if all the computations
				// were done over the original model: study the posibility of having a contant data for
				// optimize loading from memmory... it could make faster the process.
				currentRig->airSkin->resetDeformations();

				computeWeights();

				//1.Process rig->anim mode
				currentRig->saveRestPoses();

				for (int jointIdx = 0; jointIdx < currentRig->defRig.defGroups.size(); jointIdx++)
				{
					currentRig->defRig.defGroups[jointIdx]->transformation->copyBasicData(pose[currentRig->defRig.defGroups[jointIdx]->nodeId], true);
				}

				for (int rootIdx = 0; rootIdx < sktCr->parentRig->defRig.roots.size(); rootIdx++)
				{
					sktCr->parentRig->defRig.roots[rootIdx]->computeWorldPos(sktCr->parentRig->defRig.roots[rootIdx]);
				}

				sktCr->parentRig->dirtyFlag = true;
				sktCr->parentRig->update();

			}
			else
			{
				// Transform the object
				ToolManip.applyTransformation(selMgr.selection[0], ToolManip.type, ToolManip.mode);

				selMgr.selection[0]->dirtyFlag = true;
				selMgr.selection[0]->update();
			}

			return;
		}
		else if(!pressed && selMgr.currentTool == CTX_CREATE_SKT && 
				((sktCr->state == SKT_CR_IDDLE && sktCr->mode == SKT_CREATE) || 
				(!sktCr->mode == SKT_CREATE) ))
		{
			// Highlight el nodo que salga en el trazado de rayos
			beginSelection(e->pos());
			drawWithNames();
			int node = getSelection();

			AirRig* rig = (AirRig*)escena->rig;
			rig->highlight(node, true);

		}


		//Vector2f desplMouse(e->pos().x(), e->pos().y());

		// Si el raton se ha movido mas de 15 pixels consideramos que
		// quiere rotar el modelo
		//if((desplMouse-pressMouse).norm()>100.0)
		//	dragged = true;
	}
	else
		AdriViewer::mouseMoveEvent(e);
}

void GLWidget::mouseReleaseEvent(QMouseEvent* e)
{
	if(!X_ALT_modifier)
	{
		if(e->button() == Qt::LeftButton)
		{
			//printf("released Left Button\n");
			Vector2i releaseMouse(e->pos().x(), e->pos().y());
			pressed = false;

			if(ToolManip.bModifierMode)//node >= 0 && node <10)
			{
				// Estabamos modificando y hemos acabado...
				ToolManip.bModifierMode = false;

				if (!m_bRTInteraction && (sktCr->mode == SKT_CREATE || sktCr->mode == SKT_RIGG))
				{
					// Aqui podemos hacer un calcula si se ha movido algo
					//1.Process rig->anim mode
					AirRig* currentRig = ((AirRig*)sktCr->parentRig);
					currentRig->saveRestPoses();

					computeWeights();

					//2.Process anim->rig mode
					//currentRig->restorePoses();
					//currentRig->airSkin->resetDeformations();

					paintModelWithData();
				}

				return;
			}

			if(selMgr.currentTool == CTX_TRANSFORMATION && !dragged)
			{
				beginSelection(e->pos());
				drawWithNames();
				int node = getSelection();

				if(selMgr.selection.size() == 0) // No hay nada seleccionado
				{
					int nodeIdx = -1;
					// Ver si es un objeto de la escena
					for(int tempIdx = 0; tempIdx < escena->models.size(); tempIdx++)
					{
						if(node == escena->models[tempIdx]->nodeId)
						{
							// Hemos encontrado un objeto
							nodeIdx = tempIdx;
							break;
						}
					}

					if(nodeIdx >= 0 && nodeIdx < escena->models.size()) // The model exists
					{
						// Trigger selection
						selMgr.selection.push_back(escena->models[nodeIdx]);
						escena->models[nodeIdx]->select(true, escena->models[nodeIdx]->nodeId);

						object* m = escena->models[nodeIdx];

						ToolManip.bModifierMode = false;
						ToolManip.bEnable = true;
						ToolManip.setFrame(m->pos, m->qrot, Vector3d(1,1,1));
				
						printf("Model %d selected\n", node);

					}
				}
				else // CTX_CREATE
				{
					if(node > 0)
					{
						// Si es diferente del que había seleccionado-> cambiar
						if(selMgr.selection[0]->nodeId == node)
						{
							// es el mismo y no hacemos nada
						}
						else
						{
							// deseleccion
							int oldId = selMgr.selection[0]->nodeId;
							selMgr.selection[0]->select(false, oldId);
							selMgr.selection.clear();

							//seleccionar el nuevo
							int nodeIdx = -1;
							for(int tempIdx = 0; tempIdx < escena->models.size(); tempIdx++)
							{
								if(node == escena->models[tempIdx]->nodeId)
								{
									nodeIdx = tempIdx;
									break;
								}
							}

							if(nodeIdx >= 0 && nodeIdx < escena->models.size())
							{
								// Selection
								selMgr.selection.push_back(escena->models[nodeIdx]);
								escena->models[nodeIdx]->select(true, escena->models[nodeIdx]->nodeId);

								object* m = escena->models[nodeIdx];

								ToolManip.bModifierMode = false;
								ToolManip.bEnable = true;
								ToolManip.setFrame(m->pos, m->qrot, Vector3d(1,1,1));
				
								printf("Cambio de modelo: new model %d selected\n", node);
							}
						}
					}
					else
					{
						if(!dragged && selMgr.selection.size() > 0)
						{
							// Si no es nada-> y no es rotar... -> deseleccionar
							// deseleccion
							int oldId = selMgr.selection[0]->nodeId;

							selMgr.selection[0]->select(false, oldId);
							selMgr.selection.clear();

							ToolManip.bModifierMode = false;
							ToolManip.bEnable = false;
						}
					}
				}
			}
			else if(selMgr.currentTool == CTX_CREATE_SKT && !dragged)
			{
				//Sino ver si es un modelo.
				beginSelection(e->pos());
				drawWithNames();
				int node = getSelection();

				// Si no hay seleccion previa 
				if(sktCr->state == SKT_CR_IDDLE)
				{
					// Buscamos si ha seleccionado algo
					AirRig* rig = (AirRig*)escena->rig;
					DefGroup* dg = NULL;
					for(int i = 0; i < rig->defRig.defGroups.size(); i++)
					{
						if(node == rig->defRig.defGroups[i]->nodeId)
						{
							dg = rig->defRig.defGroups[i];
						}
					}

					if(dg != NULL) // Hay seleccion
					{
						// Si estamos en modo creacion -> creamos
						if(sktCr->mode == SKT_CREATE)
						{
							// Seleccionamos un nodo del rigg que ya existe
							sktCr->parentNode = dg;
							sktCr->parentRig = (AirRig*)escena->rig;
							sktCr->parentNode->select(true, sktCr->parentNode->nodeId);
							

							// Add the the existing joint to the temporal dynamic skeleton
							DefGroup* newDg = sktCr->addNewNode(dg->getTranslation(false));

							if (sktCr->state != SKT_CR_SELECTED)
							{
								
							}
							sktCr->state = SKT_CR_SELECTED;

							/*
							for(int i = 0; i< selMgr.selection.size(); i++)
							{
								if(selMgr.selection[i])
									selMgr.selection[i]->select(false, selMgr.selection[i]->nodeId);
							}*/
							vector<unsigned int > lst;
							lst.push_back(sktCr->parentNode->nodeId);
							selectElements(lst);
					
							//selMgr.selection.clear();
							selMgr.selection.push_back(sktCr->parentNode);

							return;
						}
						else // Modo animacion o test -> seleccion y cargar manipulador
						{
							vector<unsigned int > lst;
							lst.push_back(dg->nodeId);
							selectElements(lst);

							// Tenemos seleccionado un defGroup, lo vinculamos al manipulador...
							selMgr.selection.push_back(dg);
							dg->select(true, dg->nodeId);
							sktCr->state = SKT_CR_SELECTED;

							ToolManip.bModifierMode = false;
							ToolManip.bEnable = true;
							ToolManip.setFrame(dg->getTranslation(false), 
											   dg->getRotation(false), Vector3d(1,1,1));
				

							//printf("DefGroup %d selected\n", node);
						}
					}
					else // no se ha seleccionado nada
					{
						if(sktCr->mode == SKT_CREATE)
						{
							Vector2i screenPt(e->pos().x(), e->pos().y());
							Geometry* geom = (Geometry*)(escena->models[0]);

							vector<Vector3d> intersecPoints;
							Vector3d rayDir;
							vector<int> triangleIdx;

							Vector3d jointPoint;

							// Get intersections
							traceRayToObject(geom, screenPt, rayDir, intersecPoints, triangleIdx);

							if(intersecPoints.size() > 0)
							{
								// Select the right point
								if (!getFirstMidPoint(geom, rayDir, intersecPoints, triangleIdx, jointPoint))
									return;

								// Add the new joint to the temporal dynamic skeleton
								sktCr->addNewNode(jointPoint);

								// Pasamos a modo en que ya hemos seleccionado un elem.
								sktCr->state = SKT_CR_SELECTED;

								AirRig* rig = (AirRig*)escena->rig;
								if(!rig)
								{
									// He tenido que crearlo de nuevo
									escena->rig = new AirRig(scene::getNewId());
								}
								else
								{
									// Simplemente guardarme la referencia para continuar
									sktCr->parentRig = rig;
								}
							}
						}
					}
				}
				else
				{
					// Ya había algo seleccionado, por lo que tenemos que actuar en consecuencia
					if(sktCr->mode == SKT_CREATE)
					{
						// Creo un nuevo elemento... talcu
						Vector2i screenPt(e->pos().x(), e->pos().y());
						Geometry* geom = (Geometry*)(escena->models[0]);

						vector<Vector3d> intersecPoints;
						Vector3d rayDir;
						vector<int> triangleIdx;

						Vector3d jointPoint;

						// Get intersections
						traceRayToObject(geom, screenPt, rayDir, intersecPoints, triangleIdx);

						if(intersecPoints.size() > 0)
						{
							// Select the right point
							if (!getFirstMidPoint(geom, rayDir, intersecPoints, triangleIdx, jointPoint))
								return;

							// Add the new joint to the temporal dynamic skeleton
							sktCr->addNewNode(jointPoint);

							printf("TODO: Ahora deberiamos acabar la insercion\n");

							// Pasamos a modo en que ya hemos seleccionado un elem.
							sktCr->state = SKT_CR_SELECTED;

							AirRig* rig = (AirRig*)escena->rig;
							if(!rig)
							{
								// He tenido que crearlo de nuevo
								escena->rig = new AirRig(scene::getNewId());
							}
							else
							{
								// Simplemente guardarme la referencia para continuar
								sktCr->parentRig = rig;
							}
						}
					}
					else
					{
						// Buscamos si ha seleccionado algo
						AirRig* rig = (AirRig*)escena->rig;
						DefGroup* dg = NULL;
						for(int i = 0; i < rig->defRig.defGroups.size(); i++)
						{
							if(node == rig->defRig.defGroups[i]->nodeId)
							{
								dg = rig->defRig.defGroups[i];
							}
						}

						// Deseleccion
						if(selMgr.selection.size() > 0)
						{
							selMgr.selection[0]->select(false, selMgr.selection[0]->nodeId);
							selMgr.selection.clear();
						}
				
						ToolManip.bModifierMode = false;
						ToolManip.bEnable = false;

						sktCr->state = SKT_CR_IDDLE;

						vector<unsigned int > lst;
						//printf("DefGroup deselected\n");

						// Cambio la seleccion
						if(dg != NULL)
						{
							dg->select(true, dg->nodeId);
							lst.push_back(dg->nodeId);
							selectElements(lst);

							selMgr.selection.push_back(dg);

							ToolManip.bModifierMode = false;
							ToolManip.bEnable = true;
							ToolManip.setFrame(dg->getTranslation(false), dg->getRotation(false), Vector3d(1,1,1));
							
							sktCr->state = SKT_CR_SELECTED;

							//printf("Nueva Seleccion: %d\n", dg->nodeId);
						}
						else
						{
							selectElements(lst);
						}
					}
				}
			}
		}

	}


	AdriViewer::mouseReleaseEvent(e);
	
	pressed = false;
	dragged = false;

}

bool GLWidget::getFirstMidPoint(Geometry* geom, Vector3d& rayDir, vector<Vector3d>& intersecPoints, vector<int>& triangleIdx, Vector3d& point)
{
  int firstIn = -1;
  int firstOut = -1;
  // A partir del vector ordenado por profundidades cojo el primer rango in/out, para eso usamos la normal
  // del triangulo -vs- la direccion del rayo
  for(int intrIdx = 0; intrIdx < triangleIdx.size(); intrIdx++)
  {
	  float orient = rayDir.dot(geom->faceNormals[triangleIdx[intrIdx]]);
	  if(orient >= 0)
	  {
		  if(firstIn >= 0)
			  firstOut = intrIdx;
		  
		  if(firstIn >= 0 && firstOut > 0)
			  break;
	  }
	  else
	  {
		  if(firstIn < 0) firstIn = intrIdx;
	  }
  }

  if(firstIn>=0 && firstOut>0)
  {
	//printf("El punto seleccionado es...");
	point = Vector3d((intersecPoints[firstIn] + intersecPoints[firstOut])/2);
	Vector3d dir = (intersecPoints[firstOut] - intersecPoints[firstIn]);

	//printf("Rayo: %f %f %f\n", dir.x(), dir.y(), dir.z());
	//printf("%f %f %f\n", point.x(), point.y(), point.z());

	return true;
  }
  else
  {
	  printf("Ojo!: No hay punto... revisa que no haya pasado nada extrano.\n");
	  return false;
  }

}

 void GLWidget::traceRayToObject(Geometry* geom, 
								 Vector2i& pt , 
								 Vector3d& rayDir, 
								 vector<Vector3d>& intersecPoints, 
								 vector<int>& triangleIdx)
 {
  
  qglviewer::Vec orig, dir, selectedPoint;
  
  // Compute orig and dir, used to draw a representation of the intersecting line
  camera()->convertClickToLine(QPoint(pt.x(), pt.y()), orig, dir);

  // Find the selectedPoint coordinates, using camera()->pointUnderPixel().
  bool found;
  selectedPoint = camera()->pointUnderPixel(QPoint(pt.x(), pt.y()), found);

  // Construimos un rayo suficientemente largo
  Ray r;
  Vector3d origenPt = Vector3d(orig[0],orig[1],orig[2]);
  Vector3d selPt = Vector3d(selectedPoint[0],selectedPoint[1],selectedPoint[2]);
  r.P0 = origenPt;
  r.P1 = (selPt-origenPt)*10+origenPt;

  rayDir = Vector3d(dir.x, dir.y, dir.z); 

  // Recorremos todos los triángulos y evaluamos si intersecta, en ese caso lo apilamos.
  vector<unsigned int> ids;
  TriangleAux tr;

  vector<float> dephts;

  for(int trIdx = 0; trIdx< geom->triangles.size(); trIdx++ )
  {
	int pIdx = geom->triangles[trIdx]->verts[0]->id;
	tr.V0 = geom->nodes[pIdx]->position;

	pIdx = geom->triangles[trIdx]->verts[1]->id;
	tr.V1 = geom->nodes[pIdx]->position;

 	pIdx = geom->triangles[trIdx]->verts[2]->id;
	tr.V2 = geom->nodes[pIdx]->position; 

	Vector3d I;
	int res = intersect3D_RayTriangle(r, tr, I);
  
	if(res > 0 && res < 2)
	{
		printf("Interseccion con: %d\n", trIdx);
		
		float depth = (origenPt-I).norm();

		vector<float>::iterator it = dephts.begin();
		vector<Vector3d>::iterator it2 = intersecPoints.begin();
		vector<int>::iterator it3 = triangleIdx.begin();

		bool inserted = false;
		// Lo anadimos a la lista de intersecciones, pero ordenado por profundidad.
		for(int dIdx = 0; dIdx< dephts.size(); dIdx++)
		{
			if(dephts[dIdx] > depth)
			{
				dephts.insert(it, depth);
				intersecPoints.insert(it2, I);
				triangleIdx.insert(it3, trIdx);
				inserted = true;
				break;
			}

			++it2;
			++it;
		}

		if(!inserted)
		{
			dephts.push_back(depth);
			intersecPoints.push_back(I);
			triangleIdx.push_back(trIdx);
		}
	}
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

 void LoadBuffersWithModel(Modelo* m)
 {

 }


 void GLWidget::readSnakes(string fileName, string name, string path)
 {
	QString newPath( path.c_str());
	newPath = newPath +"/";

	QFile modelDefFile((fileName).c_str());
	if(modelDefFile.exists())
	{
	modelDefFile.open(QFile::ReadOnly);
	QTextStream in(&modelDefFile);

	QString sModelFile = in.readLine();
	QString sSceneName = in.readLine();

	// Leer modelo
	readModel( (newPath+sModelFile).toStdString(), sSceneName.toStdString(), newPath.toStdString());

	int count = in.readLine().toInt();

	QStringList flags = in.readLine().split(" ");
			
	vector<QString> names(count*3+1);
	vector<Vector3d> points(count*3+1);

	names[0] = flags[0];
	points[0] = Vector3d(flags[1].toDouble(), flags[2].toDouble(), flags[3].toDouble());

	for(int idx = 0 ; idx < count*3 ; idx++)
	{
		QStringList flags2 = in.readLine().split(" ");
		names[idx+1] = flags2[0];
		points[idx+1] = Vector3d(flags2[1].toDouble(), flags2[2].toDouble(), flags2[3].toDouble());
	}

	escena->skeletons.resize(1);
	escena->skeletons[0] = new skeleton(scene::getNewId());
	skeleton* skt = escena->skeletons[0];

	skt->root = new joint(scene::getNewId(T_BONE));
	skt->root->sName = names[0].toStdString();
	skt->root->worldPosition = points[0];
	skt->joints.push_back(skt->root);
	skt->jointRef[skt->root->nodeId] = skt->root;

	skt->root->pos = skt->root->worldPosition;

	for(int i = 0; i< count; i++)
	{
		QString baseName = names[i*3+1];
		QString morroName = names[i*3+2];
		QString mandibulaName = names[i*3+3];

		Vector3d basePoint = points[i*3+1];
		Vector3d morroPoint = points[i*3+2];
		Vector3d mandibulaPoint = points[i*3+3];

		joint* jt = new joint(skt->root, scene::getNewId(T_BONE));
		jt->sName = baseName.toStdString();
		jt->worldPosition = basePoint;
		jt->dirtyFlag = true;
		skt->root->childs.push_back(jt);
		skt->joints.push_back(jt);
		skt->jointRef[jt->nodeId] = jt;

		jt->pos = jt->worldPosition - skt->root->worldPosition;

		joint* lastJt = jt;

		int numOfDivisions = 22;

		Vector3d dir = morroPoint - basePoint;
		Vector3d incr = dir/(numOfDivisions+2);

		for(int j = 0; j< numOfDivisions; j++)
		{
			joint* jt2 = new joint(lastJt, scene::getNewId(T_BONE));
			jt2->sName = (baseName + QString("_00%1").arg(j)).toStdString();
			jt2->worldPosition = basePoint + incr*(j+1);			
			jt2->dirtyFlag = true;
			lastJt->childs.push_back(jt2);
			jt2->pos = jt2->worldPosition - lastJt->worldPosition;
			lastJt = jt2;

			skt->joints.push_back(jt2);
			skt->jointRef[jt2->nodeId] = jt2;
		}

		joint* jt2 = new joint(lastJt, scene::getNewId(T_BONE));
		jt2->sName = morroName.toStdString();
		jt2->worldPosition = morroPoint;
		jt2->pos = jt2->worldPosition - lastJt->worldPosition;
		jt2->dirtyFlag = true;
		lastJt->childs.push_back(jt2);
		skt->joints.push_back(jt2);
		skt->jointRef[jt2->nodeId] = jt2;

		joint* jt3 = new joint(lastJt, scene::getNewId(T_BONE));
		jt3->sName = mandibulaName.toStdString();
		jt3->worldPosition = mandibulaPoint;
		jt3->pos = jt3->worldPosition - lastJt->worldPosition;
		jt3->dirtyFlag = true;
		lastJt->childs.push_back(jt3);
		skt->joints.push_back(jt3);
		skt->jointRef[jt3->nodeId] = jt3;
	}

	skt->dirtyFlag = true;
	skt->propagateDirtyness();

	skt->root->computeWorldPos();
	skt->root->computeRestPos();

	//ComputeWithWorldOrientedRotations(skt->root);

	}
		
	updateSceneView(); 
}


 void GLWidget::readScene(string fileName, string name, string path)
 {
	 // temporal para construir la serpiente
	 if(QString(fileName.c_str()).right(3) == "snk")
	 {
		readSnakes(fileName, name, path);
		return;
	 }
	 
	 // Proceso normal
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
        m->sPath = newPath.toStdString(); // importatente para futuras referencias
		BuildSurfaceGraphs(m);

		// Save all the pieces.
		/*
		if(m->bind->surfaces.size() > 1)
		{
			for(int sf = 0; sf < m->bind->surfaces.size(); sf++)
			{
				QString sFileName = QString("%1model_piece_%2_%3.off").arg(m->sPath.c_str()).arg(sf).arg(m->sName.c_str());
				SaveOFFFromSurface(m->bind->surfaces[sf], sFileName.toStdString());

				for(int sfNode = 0; sfNode < m->bind->surfaces[sf].nodes.size(); sfNode++)
				{
					if(	m->bind->pointData[m->bind->surfaces[sf].nodes[sfNode]->id].isBorder)
					{
						printf(">>> %s\n", sFileName.toStdString().c_str());
						break;

					}
				}
			}
		}
		*/

		VoxelizeModel(m, false);

		getBDEmbedding(m);

		// De momento lo dejamos.
		//LoadBuffersWithModel(m);

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

			//((AirRig*)escena->rig)->airSkin->computeRestPositions(escena->skeletons);

			for(int ro = 0; ro < ((AirRig*)escena->rig)->defRig.roots.size(); ro++)
			{
				((AirRig*)escena->rig)->defRig.roots[ro]->computeRestPos(((AirRig*)escena->rig)->defRig.roots[ro]);
			}

			((AirRig*)escena->rig)->enableDeformation = true;
		}

		paintModelWithData();

    }
 }

 // apilar nuevo shader
 void GLWidget::pushShader(shaderIdx newShader)
 {
	 setShaderConfiguration(newShader);
	 m_pShadersPile.push_back(newShader);
 }

 // restaurar el ultimo shader, si hay
 void GLWidget::popShader()
 {
	 if (m_pShadersPile.size() > 0)
	 {
		 shaderIdx oldShader = m_pShadersPile.back();
		 m_pShadersPile.pop_back();
		 setShaderConfiguration(oldShader);
	 }
	 else
	 {
		// Apilar uno por defecto
		 setShaderConfiguration(SHD_BASIC);
		 
		 printf("[ERROR] Estas desapilando un shader que no existe... te has descontado\n");
	 }
 }

void GLWidget::setAuxValue(int value)
{ 
	valueAux = value;
	for (int i = 0; i < escena->models.size(); i++)
	{
		Modelo* m = (Modelo*)escena->models[i];
	
		m->shading->setAuxValue(value);
	}
}