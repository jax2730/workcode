///////////////////////////////////////////////////////////////////////////////
//
// @file HeatMapExploreWidget.h
// @author luoshuquan
// @date 2020/09/27 Sunday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __HeatMapExploreWidget_H__
#define __HeatMapExploreWidget_H__

#include <QWidget> 

class HeatMapExploreWidget : public QWidget
{
	Q_OBJECT

public:
	HeatMapExploreWidget(QWidget * parent = nullptr);
	~HeatMapExploreWidget();

protected slots:
	void onToolButtonClicked();
	void onCheckBoxStateChanged(int);
	void onSliderValueChanged(int);
	void onTableWidgetItemClicked(QTableWidgetItem*);
	void onTableWidgetItemDoubleClicked(QTableWidgetItem*);

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent * event) override;

private:
	class HeatMapExploreWidgetPrivate *dp = nullptr;

}; // class HeatMapExploreWidget 

#endif // __HeatMapExploreWidget_H__
