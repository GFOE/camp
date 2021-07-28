#include "nav_source.h"
#include <tf2/utils.h>
#include <QPainter>

NavSource::NavSource(const project11_msgs::NavSource& source, QObject* parent, QGraphicsItem *parentItem): QObject(parent), GeoGraphicsItem(parentItem)
{
  ros::NodeHandle nh;
  m_position_sub = nh.subscribe(source.position_topic, 1, &NavSource::positionCallback, this);
  m_orientation_sub = nh.subscribe(source.orientation_topic, 1, &NavSource::orientationCallback, this);
  setObjectName(source.name.c_str());
}

QRectF NavSource::boundingRect() const
{
  return shape().boundingRect().marginsAdded(QMargins(2,2,2,2));
}

void NavSource::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  painter->save();

  QPen p;
  p.setCosmetic(true);
  p.setColor(Qt::red);
  p.setWidth(2);
  painter->setPen(p);
  painter->drawPath(shape());

  painter->restore();
}

QPainterPath NavSource::shape() const
{
  QPainterPath ret;
  if(!m_location_history.empty())
  {
    ret.moveTo(m_location_history.front().pos);
    for (auto location: m_location_history)
      ret.lineTo(location.pos);
  }
  return ret;
}

void NavSource::positionCallback(const sensor_msgs::NavSatFix::ConstPtr& message)
{
  QGeoCoordinate position(message->latitude, message->longitude, message->altitude);
  QMetaObject::invokeMethod(this,"updateLocation", Qt::QueuedConnection, Q_ARG(QGeoCoordinate, position));
}


void NavSource::orientationCallback(const sensor_msgs::Imu::ConstPtr& message)
{
  double yaw = tf2::getYaw(message->orientation);
  double heading = 90-180*yaw/M_PI;
  QMetaObject::invokeMethod(this,"updateHeading", Qt::QueuedConnection, Q_ARG(double, heading));
}

void NavSource::updateLocation(QGeoCoordinate const &location)
{
  emit beforeNavUpdate();
  prepareGeometryChange();
  LocationPosition lp;
  lp.location = location;
  auto bg = findParentBackgroundRaster();
  if(bg)
    lp.pos = geoToPixel(location, bg);
  m_location_history.push_back(lp);
  m_location = lp;
  while(m_max_history > 0 && m_location_history.size() > m_max_history)
    m_location_history.pop_front();
}

void NavSource::updateHeading(double heading)
{
  emit beforeNavUpdate();
  prepareGeometryChange();
  m_heading = heading;
}

void NavSource::updateProjectedPoints()
{
  prepareGeometryChange();
  auto bg = findParentBackgroundRaster();
  if(bg)
    for(auto& lp: m_location_history)
      lp.pos = geoToPixel(lp.location, bg);
}

const LocationPosition& NavSource::location() const
{
  return m_location;
}

double NavSource::heading() const
{
  return m_heading;
}

void NavSource::setMaxHistory(int max_history)
{
  m_max_history = max_history;
}
