#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui/qevent.h>

#include <gui/treeitem.h>
#include <gui/treemodel.h>
#include <gui/AirOutliner.h>

MainWindow::MainWindow(QWidget *parent) : AdriMainWindow(parent)
{
	ui->setupUi(this);

	ui->verticalLayout_4->removeWidget(ui->glCustomWidget);
	ui->glCustomWidget = new GLWidget(ui->frame);

	ui->glCustomWidget->setObjectName(QStringLiteral("glCustomWidget"));
	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(ui->glCustomWidget->sizePolicy().hasHeightForWidth());
	ui->glCustomWidget->setSizePolicy(sizePolicy);
	ui->verticalLayout_4->addWidget(ui->glCustomWidget);
	widget = (GLWidget*)ui->glCustomWidget;

	widget->parent = this;


	connect(ui->voxelization_btn, SIGNAL(released()), this, SLOT(Compute()));
	connect(ui->auxValueInt, SIGNAL(valueChanged(int)), this, SLOT(changeAuxValueInt(int)));

	connect(ui->smoothingPasses, SIGNAL(valueChanged(int)), this, SLOT(changeLocalSmoothingPasses(int)));
	connect(ui->localSmoothingPasses, SIGNAL(valueChanged(int)), this, SLOT(changeGlobalSmoothingPasses(int)));

    connect(ui->prop_function_updt, SIGNAL(released()), widget, SLOT(PropFunctionConf()));

    connect(ui->paintModel_btn, SIGNAL(released()), widget, SLOT(paintModelWithGrid()));
    connect(ui->metricUsedCheck, SIGNAL(released()), widget, SLOT(PropFunctionConf()));

    connect(ui->drawInfluences_check, SIGNAL(released()), widget, SLOT(showHCoordinatesSlot()));

    connect(ui->coordTab, SIGNAL(currentChanged(int)), widget, SLOT(active_GC_vs_HC(int)));

    connect(ui->glCustomWidget, SIGNAL(updateSceneView()), this, SLOT(updateSceneView()));
    connect(ui->outlinerView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectObject(QModelIndex)));

    connect(ui->segmentation_btn, SIGNAL(toggled(bool)), this, SLOT(toogleToShowSegmentation(bool)));

    connect(ui->exportWeights_btn, SIGNAL(released()), widget, SLOT(exportWeightsToMaya()));

    connect(ui->expansionSlider, SIGNAL(sliderReleased()), this, SLOT(changeExpansionSlider()));
    connect(ui->expansionSlider, SIGNAL(valueChanged(int)), this, SLOT(updateExpansionSlidervalue(int)));

	connect(ui->thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(updateThresholdSlidervalue(int)));
	connect(ui->threshold_enable, SIGNAL(toggled(bool)), this, SLOT(enableThreshold(bool)));
	connect(ui->threshold_enable_adaptative, SIGNAL(toggled(bool)), this, SLOT(enableAdaptativeThreshold(bool)));
	
    //connect(ui->smoothPropagationSlider, SIGNAL(sliderReleased()), this, SLOT(changeSmoothSlider()));
    //connect(ui->smoothPropagationSlider, SIGNAL(valueChanged(int)), this, SLOT(updateSmoothSlidervalue(int)));

	connect(ui->auxValueInt, SIGNAL(valueChanged(int)), this, SLOT(changeAuxValueInt(int)));

    connect(ui->glCustomWidget, SIGNAL(jointDataShow(float, int)), this , SLOT(jointDataUpdate(float,int)));

	connect(ui->ip_axisX, SIGNAL(valueChanged(int)), this, SLOT(changeInteriorPointPosition()));
	connect(ui->ip_axisY, SIGNAL(valueChanged(int)), this, SLOT(changeInteriorPointPosition()));
	connect(ui->ip_axisZ, SIGNAL(valueChanged(int)), this, SLOT(changeInteriorPointPosition()));

	connect(ui->paintModel, SIGNAL(clicked()), this, SLOT(updateModelColors()));
	connect(ui->paintPlane, SIGNAL(clicked()), this, SLOT(updateClipingPlaneColor()));

	connect(ui->drawPlaneCheck, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->PlaneOrientX, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->planeOrientY, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->planeOrientZ, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->PlaneDataCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeVisModeForPlane(int)));
	
	connect(ui->positionPlaneSlider, SIGNAL(sliderMoved(int)), this, SLOT(changeSelPointForPlane(int)));

	connectSignals();

}

void MainWindow::Compute()
{
	widget->computeProcess();
}

MainWindow::~MainWindow()
{
}


void MainWindow::updateSceneView()
{
    //treeViewModel->clear(); // Limpiamos la lista.


    TreeModel* treeViewModel;
    treeViewModel = new TreeModel();
    ui->outlinerView->setModel(treeViewModel);

    treeNode* root = new treeNode();
	AirOutliner outl(ui->glCustomWidget->escena);
    outl.getSceneTree(root);

    QList<TreeItem*> parents;
    parents << treeViewModel->rootItem;
    AdriMainWindow::updateSceneView(parents.back(), root);

    emit ui->outlinerView->dataChanged(ui->outlinerView->rootIndex(), ui->outlinerView->rootIndex());
    repaint();
    // Hay que actualizar la vista para que aparezca todo.
}

void MainWindow::UpdateScene()
{
	//widget->paintModelWithData();
}

/*
void MainWindow::changeSmoothSlider()
{
    float valueAux = ui->smoothPropagationSlider->value();
    float value = valueAux/10.0;

    ui->smoothPropagationEdit->setText(QString("%1").arg(value));
    widget->changeSmoothPropagationDistanceRatio(value);
}
*/

void MainWindow::changeGlobalSmoothingPasses(int value)
{
    float valueAux = ui->smoothingPasses->value();
    widget->setGlobalSmoothPasses(valueAux);
}

void MainWindow::changeLocalSmoothingPasses(int value)
{
    float valueAux = ui->smoothingPasses->value();
    widget->setLocalSmoothPasses(valueAux);
}

void MainWindow::changeAuxValueInt(int value)
{
    widget->valueAux = value;
	widget->paintModelWithData();
	widget->updateGridRender();
}

void MainWindow::updateThresholdSlidervalue(int value)
{
	ui->threshold_edit->setText(QString("%1").arg((float)value/100.0));

	if(ui->threshold_enable->isChecked())
		widget->setThreshold((float)value/100.0);
	else
		widget->setThreshold( -10);
}
void MainWindow::enableThreshold(bool toogle)
{
	int value = ui->thresholdSlider->value();

	if(ui->threshold_enable->isChecked())
		widget->setThreshold((float)value/100.0);
	else
		widget->setThreshold( -10);
}

void MainWindow::enableAdaptativeThreshold(bool toogle)
{
	int value = ui->thresholdSlider->value();

	if(!ui->threshold_enable_adaptative->isChecked())
		enableThreshold(ui->threshold_enable->isChecked());
	else
		widget->setThreshold(0);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    int current = 0;
    bool checked = false;
    switch(event->key())
    {
    case Qt::Key_Up:
        current = ui->SliceSelectorXZ->value();
        if(current + 5 <= ui->SliceSelectorXZ->maximum())
        {
            ui->SliceSelectorXZ->setValue(current+5);
            widget->ChangeSliceXZ(current+5);
        }
        break;
    case Qt::Key_Down:
        current = ui->SliceSelectorXZ->value();
        if(current - 5 >= ui->SliceSelectorXZ->minimum())
        {
            ui->SliceSelectorXZ->setValue(current-5);
            widget->ChangeSliceXZ(current-5);
        }
        break;
    case Qt::Key_Right:
        current = ui->SliceSelectorXY->value();
        if(current + 5 <= ui->SliceSelectorXY->maximum())
        {
            ui->SliceSelectorXY->setValue(current+5);
            widget->ChangeSliceXY(current+5);
        }
        break;
    case Qt::Key_Left:
        current = ui->SliceSelectorXY->value();
        if(current - 5 >= ui->SliceSelectorXY->minimum())
        {
            ui->SliceSelectorXY->setValue(current-5);
            widget->ChangeSliceXY(current-5);
        }
        break;

	// Defined in the parent.
    //case Qt::Key_Plus:     
    //case Qt::Key_Minus:
    //case Qt::Key_0:
    //case Qt::Key_1:
    //case Qt::Key_2:
    //case Qt::Key_3:
	//case Qt::Key_4:
	//case Qt::Key_5:
    //case Qt::Key_6:
    //case Qt::Key_7:
    //case Qt::Key_8:
    //case Qt::Key_9:
    default:
		AdriMainWindow::keyPressEvent(event);
        break;
    }
}

void MainWindow::updateClipingPlaneData()
{
	double sliderPos = (float)ui->positionPlaneSlider->value()/1000.0;
	int visCombo = ui->PlaneDataCombo->currentIndex();
	int selectedVertex = ui->dataSource->text().toInt();
	bool checked = ui->drawPlaneCheck->checkState() == Qt::Checked;
	ui->positionPlaneEdit->setText(QString("%1").arg(sliderPos, 3, 'g', 3));

	int orient = 0;
	if(ui->planeOrientY->isChecked())
		orient = 1;
	else if(ui->planeOrientZ->isChecked())
		orient = 2;

	widget->setPlaneData(checked, selectedVertex,visCombo, sliderPos, orient);
}

void MainWindow::updateClipingPlaneColor()
{
	widget->paintPlaneWithData(true);
}

void MainWindow::updateModelColors()
{
	widget->paintModelWithData();
}

void MainWindow::toogleToShowSegmentation(bool toogle)
{
	widget->toogleToShowSegmentation(toogle);
}

void MainWindow::changeExpansionSlider()
{
    float valueAux = ui->expansionSlider->value();
    float value = 0;
    if(valueAux <= 100)
        value = ((float)ui->expansionSlider->value())/100.0;
    else
    {
        value = (((valueAux-100)/100)*2)+1.0;
    }

    ui->expansionValueEdit->setText(QString("%1").arg(value));
    ((GLWidget*)ui->glCustomWidget)->changeExpansionFromSelectedJoint(value);
}
