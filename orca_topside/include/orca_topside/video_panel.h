#ifndef VIDEO_PANEL_H
#define VIDEO_PANEL_H

#include "rclcpp/rclcpp.hpp"
#include "rviz_common/panel.hpp"
#include "qt5/QtCore/QtCore"

#include "QGst/Pipeline"
#include "QGst/Ui/VideoWidget"

namespace orca_topside {

class VideoPlayer : public QGst::Ui::VideoWidget
{
public:
  VideoPlayer(QWidget* parent = 0);
  ~VideoPlayer();

  virtual QSize sizeHint() const { return QSize(400, 400); }

protected:
  QGst::PipelinePtr pipeline_;

  void onBusMessage(const QGst::MessagePtr& message);
};

class VideoPanel : public rviz_common::Panel
{
public:
  VideoPanel(QWidget* parent = 0);
  ~VideoPanel();

protected:
  VideoPlayer* player_;
};

} // namespace orca_topside

#endif // VIDEO_PANEL_H
