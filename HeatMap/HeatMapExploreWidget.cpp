///////////////////////////////////////////////////////////////////////////////
//
// @file HeatMapExploreWidget.cpp
// @author luoshuquan
// @date 2020/09/27 Sunday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "SceneBoxPrecompiled.h"
#include "HeatMapExploreWidget.h"
#include "ui_HeatMapExploreWidget.h"
#include "QtWidgetUtil.h"
#include "QtGraphicsItemEx.h"

#include "ITerrainEditor.h"
#include "OgitorsRoot.h"
#include "ViewportEditor.h"
#include "CameraEditor.h"
#include "SceneManagerEditor.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGuiApplication>

#include "VegetationAreaEditor.h"
#include "EchoVegetationArea.h"
#include "EchoVegetationAreaResource.h"

class HeatMapExploreWidgetPrivate
{
public:
	Ui::HeatMapExploreWidget ui;

	QGraphicsScene * mScene = nullptr;
	class HeatMapView * mView = nullptr;
	std::unique_ptr<QGraphicsPixmapItem> mHeatmapItem;
	std::vector<std::unique_ptr<QGraphicsPolygonItem>> mVegetationAreas;
	std::vector<std::unique_ptr<GraphicsImageItem>> mTerrThumbnails;
	Echo::Image mTerrainDiffuse;

}; // class HeatMapExploreWidgetPrivate

///////////////////////////////////////////////////////////////////////////////
// GraphicsView

class HeatMapView : public QGraphicsView
{
public:
	HeatMapView(QWidget* parent = 0)
		:QGraphicsView(parent)
	{}
	~HeatMapView() {}

protected:
	virtual void wheelEvent(QWheelEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
private:
	QPoint mOldPoint;
	QVector<QGraphicsItem*> mSelItems;
};
void HeatMapView::wheelEvent(QWheelEvent *event)
{
	float fScaleBase = 1.05f;
	if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
		fScaleBase = 1.15f;

	float fScale = fScaleBase;
	if (event->delta() < 0)
		fScale = 1 / fScaleBase;
	scale(fScale, fScale);
}

void HeatMapView::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::RightButton)
	{
		setCursor(QCursor(Qt::ClosedHandCursor));
		QScrollBar* pHBar = horizontalScrollBar();
		QPoint offset = event->pos() - mOldPoint;
		if (pHBar != 0) {
			pHBar->setSliderPosition(pHBar->sliderPosition() - offset.x());
		}
		QScrollBar* pVBar = verticalScrollBar();
		if (pVBar) {
			pVBar->setSliderPosition(pVBar->sliderPosition() - offset.y());
		}
		mOldPoint = event->pos();
	}
}

void HeatMapView::mousePressEvent(QMouseEvent *event)
{
	mOldPoint = event->pos();
	if (event->buttons() & Qt::RightButton)
	{
		setCursor(QCursor(Qt::OpenHandCursor));
	}
}

void HeatMapView::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton) {
		unsetCursor();
	}
}
///////////////////////////////////////////////////////////////////////////////

HeatMapExploreWidget::HeatMapExploreWidget(QWidget * parent /* = nullptr */)
	: QWidget(parent, Qt::Window)
	, dp(new HeatMapExploreWidgetPrivate)
{
	dp->ui.setupUi(this);

	dp->mView = new HeatMapView(dp->ui.widgetPanel);
	dp->mView->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
	dp->mView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	dp->mScene = new QGraphicsScene();
	dp->mView->setScene(dp->mScene);

	QHBoxLayout * pLayout = new QHBoxLayout();
	pLayout->addWidget(dp->mView);
	pLayout->setMargin(0);
	dp->ui.widgetPanel->setLayout(pLayout);

	dp->ui.tbtnSelHeatmap->setIcon(QIcon(":/icons/fileopen.svg"));
	dp->ui.tbtnVegetationArea->setIcon(QIcon(":/icons/vegetation_area.png"));

	connect(dp->ui.tbtnSelHeatmap, SIGNAL(clicked()),
			this, SLOT(onToolButtonClicked()));
	connect(dp->ui.tbtnVegetationArea, SIGNAL(clicked()),
			this, SLOT(onToolButtonClicked()));
	connect(dp->ui.chkTerrainThumbnail, SIGNAL(stateChanged(int)),
			this, SLOT(onCheckBoxStateChanged(int)));
	connect(dp->ui.hsldHeatmapOpacity, SIGNAL(valueChanged(int)),
			this, SLOT(onSliderValueChanged(int)));

	QStringList headers;
	headers << "Editor" << "Type";
	dp->ui.tbwEditors->setColumnCount(headers.size());
	dp->ui.tbwEditors->setHorizontalHeaderLabels(headers);
	dp->ui.tbwEditors->setColumnWidth(0, 400);
	dp->ui.tbwEditors->setColumnWidth(1, 150);

	connect(dp->ui.tbwEditors, SIGNAL(itemClicked(QTableWidgetItem *)),
			this, SLOT(onTableWidgetItemClicked(QTableWidgetItem *)));
	connect(dp->ui.tbwEditors, SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
			this, SLOT(onTableWidgetItemDoubleClicked(QTableWidgetItem *)));

	dp->ui.splitter->setStretchFactor(0, 3);
	dp->ui.splitter->setStretchFactor(1, 1);

	QtWidgetUtil::restoreWindowGeometry(this);
}

HeatMapExploreWidget::~HeatMapExploreWidget()
{
	QtWidgetUtil::saveWindowGeometry(this);
}

void HeatMapExploreWidget::onToolButtonClicked()
{
	using namespace Echo;
	using namespace EchoBox;

	const QToolButton * pToolBtn = qobject_cast<QToolButton*>(sender());

	if (pToolBtn == dp->ui.tbtnSelHeatmap)
	{
		QString qsHeatmap = QFileDialog::getOpenFileName(this,
				QStringLiteral("热力图"), QString(), QStringLiteral("热力图 (*.png)"));
		if (qsHeatmap.isEmpty())
			return;

		Vector3 vLeftTop(0.f, 0.f, 0.f);
		if (ITerrainEditor::GetSelf() != nullptr)
		{
			FloatRect rcPageF = ITerrainEditor::GetSelf()->getRange();
			vLeftTop.x = rcPageF.left;	vLeftTop.z = rcPageF.top;
			vLeftTop /= ITerrainEditor::GetSelf()->getGridSize();
		}

		dp->mScene->removeItem(dp->mHeatmapItem.get());
		dp->mHeatmapItem.reset(new QGraphicsPixmapItem(QPixmap(qsHeatmap)));
		dp->mScene->addItem(dp->mHeatmapItem.get());
		dp->ui.ledHeatmap->setText(qsHeatmap);
		dp->mHeatmapItem->setZValue(100.f);
		dp->mHeatmapItem->setPos(vLeftTop.x, vLeftTop.z);
	}
	else if (pToolBtn == dp->ui.tbtnVegetationArea)
	{
		for (const auto & i : dp->mVegetationAreas)
			dp->mScene->removeItem(i.get());

		dp->mVegetationAreas.clear();

		const auto & lstVegAreas = OgitorsRoot::getSingletonPtr()->GetObjectsByType(ETYPE_VEGETATION_AREA);
		for (const auto & i : lstVegAreas)
		{
			VegetationAreaEditor * pVegAreaEd = dynamic_cast<VegetationAreaEditor*>(i.second);
			const VegetationArea *pVegArea = static_cast<const VegetationArea*>(pVegAreaEd->getHandle());
			const VegetationAreaResourcePtr & resPtr = pVegArea->getResource();
			const Vector3 & vPosition = resPtr->getPosition();
			const auto & pts = resPtr->getPoints();
			QPolygonF qtPolygon;
			for (const auto & p : pts) {
				qtPolygon << QPointF(p.x, p.y) / 8.f;
			}

			QGraphicsPolygonItem * pItem = new QGraphicsPolygonItem(qtPolygon);
			pItem->setPos(QPointF(vPosition.x, vPosition.z) / 8.f);
			pItem->setToolTip(resPtr->getName().c_str());
			pItem->setZValue(110.f);
			QPen pen = pItem->pen();
			pen.setColor(QColor(qrand() % 156 + 100, qrand() % 156 + 100, qrand() % 156 + 100, 200));
			pItem->setPen(pen);
			dp->mScene->addItem(pItem);
			dp->mVegetationAreas.emplace_back(pItem);
		}
	}
}

void HeatMapExploreWidget::onCheckBoxStateChanged(int state)
{
	using namespace Echo;
	using namespace EchoBox;

	for (const auto & i : dp->mTerrThumbnails)
		dp->mScene->removeItem(i.get());

	dp->mTerrThumbnails.clear();
	if (!dp->ui.chkTerrainThumbnail->isChecked())
		return;

	if (nullptr == ITerrainEditor::GetSelf())
		return;

	ITerrainEditor::GetSelf()->fillBaseDiffuse(dp->mTerrainDiffuse);
	QImage qimg(dp->mTerrainDiffuse.getData(), dp->mTerrainDiffuse.getWidth(),
			dp->mTerrainDiffuse.getHeight(), QImage::Format_RGB32);
	QRectF rect(0.f, 0.f, dp->mTerrainDiffuse.getWidth(), dp->mTerrainDiffuse.getHeight());
	GraphicsImageItem *pItem = new GraphicsImageItem(qimg, rect);
	Rect rcPage = ITerrainEditor::GetSelf()->getPageRect();
	pItem->setPos(float(rcPage.left) * ITerrainEditor::GetSelf()->getGridNum(),
				  float(rcPage.top) * ITerrainEditor::GetSelf()->getGridNum());
	pItem->setRectVisible(false);
	pItem->setZValue(0.f);
	dp->mScene->addItem(pItem);
	dp->mTerrThumbnails.emplace_back(pItem);
}

void HeatMapExploreWidget::onSliderValueChanged(int value)
{
	float fOpacity = float(value)/float(dp->ui.hsldHeatmapOpacity->maximum());
	if (dp->mHeatmapItem != nullptr)
		dp->mHeatmapItem->setOpacity(fOpacity);
}

void HeatMapExploreWidget::onTableWidgetItemClicked(QTableWidgetItem * item)
{
	if (nullptr == item)
		return;

	int row = item->row();
	QString qsEditor = dp->ui.tbwEditors->item(row, 0)->text();
	using namespace Echo;
	using namespace EchoBox;
	CBaseEditor *pEd = OgitorsRoot::getSingletonPtr()->FindObject(qsEditor.toStdString());
	if (nullptr == pEd)
		return;

	OgitorsRoot::getSingletonPtr()->GetSelection()->setSelection(pEd);
}

void HeatMapExploreWidget::onTableWidgetItemDoubleClicked(QTableWidgetItem * item)
{
	onTableWidgetItemClicked(item);
	if (item)
		EchoBox::OgitorsRoot::getSingletonPtr()->GetViewport()->FocusSelectedObject();
}

void HeatMapExploreWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (dp->mHeatmapItem != nullptr)
		{
			QPoint pt = event->pos();
			QPoint ptView = dp->mView->mapFrom(this, pt);
			if (!dp->mView->rect().contains(ptView.x(), ptView.y()))
				return;
			QPointF ptfScene = dp->mView->mapToScene(ptView);
			QPointF ptfItem = dp->mHeatmapItem->mapFromScene(ptfScene);
			QRectF rcfItem = dp->mHeatmapItem->boundingRect();
			QPointF ptOff = ptfItem - rcfItem.topLeft();

			using namespace Echo;
			using namespace EchoBox;

			Vector3 v3Pos(ptOff.x(), 0.f, ptOff.y());
			if (ITerrainEditor::GetSelf() != nullptr)
			{
				v3Pos *= ITerrainEditor::GetSelf()->getGridSize();
				FloatRect rcRange = ITerrainEditor::GetSelf()->getRange();
				v3Pos.x += rcRange.left;
				v3Pos.z += rcRange.top;
			}
			v3Pos.y = 999999999.f;

			Ray ray(v3Pos, Vector3::NEGATIVE_UNIT_Y);
			std::map<std::string, std::string> res;

			Echo::RaySceneQuery *mRaySceneQuery = OgitorsRoot::getSingletonPtr()->GetSceneManagerEditor()->getRayQuery();
			mRaySceneQuery->setRay(ray);
			mRaySceneQuery->setSortByDistance(true);
			const RaySceneQueryResult &query_result = mRaySceneQuery->execute();
			for (const auto & obj : query_result)
			{
				const auto pMovable = obj.movable;
				if (pMovable == nullptr)
					continue;

				const CBaseEditor *pEd = OgitorsRoot::getSingletonPtr()->FindObject(pMovable->getName().toString());
				if (pEd == nullptr)
					continue;

				const auto edType = pEd->getFactoryDynamic()->mEditorType;
				if (edType != ETYPE_ENTITY && edType != ETYPE_BUILDING &&
						edType != ETYPE_MOUNTAIN && edType != ETYPE_ROLE)
					continue;

				if (pEd != nullptr)
					res[pEd->getName()] = pEd->getFactoryDynamic()->getTypeName();
			}


			dp->ui.tbwEditors->blockSignals(true);
			while (dp->ui.tbwEditors->rowCount())
				dp->ui.tbwEditors->removeRow(0);

			int row = 0;
			dp->ui.tbwEditors->setSortingEnabled(false);
			for (const auto & i : res)
			{
				dp->ui.tbwEditors->insertRow(row);
				dp->ui.tbwEditors->setItem(row, 0, new QTableWidgetItem(i.first.c_str()));
				dp->ui.tbwEditors->setItem(row, 1, new QTableWidgetItem(i.second.c_str()));
				++row;
			}
			dp->ui.tbwEditors->setSortingEnabled(true);
			dp->ui.tbwEditors->blockSignals(false);

			CCameraEditor* pCamEd = OgitorsRoot::getSingletonPtr()->GetViewport()->getCameraEditor();
			Vector3 vCamPos = pCamEd->getPosition();
			float fHeight = 0.f;
			if (OgitorsRoot::getSingletonPtr()->getTerrainHeight(vCamPos, fHeight))
			{
				float fHeightDiff = vCamPos.y - fHeight;
				vCamPos = v3Pos;
				if (OgitorsRoot::getSingletonPtr()->getTerrainHeight(vCamPos, fHeight)) {
					vCamPos.y = fHeight + fHeightDiff;
				}
			}
			pCamEd->setDerivedPosition(vCamPos);
			Vector3 normal = Vector3::UNIT_Z;
			pCamEd->setDerivedOrientation(normal.getRotationTo(Vector3::UNIT_Y));

			qDebug() << pt << ptView << ptfScene << ptfItem;
		}
	}
	qDebug() << __FUNCTION__;
}

