/*
 *    Copyright (C) 2017 by YOUR NAME HERE
 *
 *    This file is part of RoboComp
 *
 *    RoboComp is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    RoboComp is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RoboComp.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
       \brief
       @author authorname
*/



#ifndef SPECIFICWORKER_H
#define SPECIFICWORKER_H

#include <genericworker.h>
#include <innermodel/innermodel.h>

#include <pcl/common/angles.h>
#include <pcl/io/openni2_grabber.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/visualization/boost.h>
#include <pcl/visualization/image_viewer.h>
#include <pcl/console/print.h>
#include <pcl/console/parse.h>
#include <pcl/console/time.h>

#include <boost/chrono.hpp>

#include <pcl/io/openni2/openni.h>



template <typename PointType> class OpenNI2Viewer
{
public:
	bool image_init, cloud_init;
	boost::signals2::connection cloud_connection;
	boost::signals2::connection image_connection;
	typedef pcl::PointCloud<PointType> Cloud;
	typedef typename Cloud::ConstPtr CloudConstPtr;

	OpenNI2Viewer (pcl::io::OpenNI2Grabber& grabber) : grabber_ (grabber), rgb_data_ (0), rgb_data_size_ (0)
	{
		readIM = new ImageMap();
		writeIM = new ImageMap();

		image_init = false;
		cloud_init = false;
		boost::function<void (const CloudConstPtr&) > cloud_cb = boost::bind (&OpenNI2Viewer::cloud_callback, this, _1);
		cloud_connection = grabber_.registerCallback (cloud_cb);
		if (grabber_.providesCallback<void (const boost::shared_ptr<pcl::io::openni2::Image>&)>())
		{
			boost::function<void (const boost::shared_ptr<pcl::io::openni2::Image>&) > image_cb = boost::bind (&OpenNI2Viewer::image_callback, this, _1);
			image_connection = grabber_.registerCallback (image_cb);
		}
		else
		{
			printf("Not providing image callbacks\n");
			exit(-1);
		}
		grabber_.start ();
	}

	void cloud_callback (const CloudConstPtr& cloud)
	{
		boost::mutex::scoped_lock lock (cloud_mutex_);
		cloud_ = cloud;
	}

	void image_callback (const boost::shared_ptr<pcl::io::openni2::Image>& image)
	{
		static bool first=true;
		Image img;
		if (image->getEncoding () != pcl::io::openni2::Image::RGB)
		{
			if (rgb_data_size_ < image->getWidth () * image->getHeight ())
			{
				if (rgb_data_)
				{
					delete [] rgb_data_;
				}
				rgb_data_size_ = image->getWidth () * image->getHeight ();
				rgb_data_ = new unsigned char [rgb_data_size_ * 3];
			}
			image_->fillRGB (image_->getWidth (), image_->getHeight (), rgb_data_);
			img.colorImage.resize(image->getWidth()*image->getHeight());
			memcpy(&img.colorImage[0], rgb_data_, image->getWidth()*image->getHeight());
		}
		else
		{
			img.colorImage.resize(image->getWidth()*image->getHeight());
			memcpy(&img.colorImage[0], (const unsigned char*)image->getData(), image->getWidth()*image->getHeight());
		}
		img.width = image->getWidth();
		img.height = image->getHeight();

		image_mutex_.lock();
		printf("cb %s %d\n", "rgbd", (int)img.colorImage.size());
		sleep(1);
		writeIM->operator[]("rbgd") = img;
		if (first)
		{
			*readIM = *writeIM;
			first = false;
		}
		std::swap(readIM, writeIM);
		image_mutex_.unlock();
	}

  void run ()
  {
    boost::shared_ptr<pcl::io::openni2::Image> image;
    // See if we can get a cloud
    if (cloud_mutex_.try_lock ())
    {
      cloud_.swap (cloud);
      cloud_mutex_.unlock ();
    }
    // See if we can get an image
    if (image_mutex_.try_lock ())
    {
      image_.swap(image);
      image_mutex_.unlock ();
    }
  }

	pcl::io::OpenNI2Grabber& grabber_;
	boost::mutex cloud_mutex_;
	boost::mutex image_mutex_;

	CloudConstPtr cloud_, cloud;
	boost::shared_ptr<pcl::io::openni2::Image> image_;
	unsigned char *rgb_data_;
	unsigned rgb_data_size_;

	ImageMap *readIM, *writeIM;

public:
	ImageMap *getImageMap()
	{
		return readIM;
	}
};



class SpecificWorker : public GenericWorker
{
Q_OBJECT
public:
	SpecificWorker(MapPrx& mprx);
	~SpecificWorker();
	bool setParams(RoboCompCommonBehavior::ParameterList params);

	CameraParamsMap getAllCameraParams();
	void getPointClouds(const CameraList &cameras, PointCloudMap &clouds);
	void getImages(const CameraList &cameras, ImageMap &images);
	void getProtoClouds(const CameraList &cameras, PointCloudMap &protoClouds);
	void getDecimatedImages(const CameraList &cameras, const int decimation, ImageMap &images);

public slots:
	void compute();

private:
	InnerModel *innerModel;


private:
	CameraParams params;
	pcl::io::OpenNI2Grabber *grabber;
	OpenNI2Viewer<pcl::PointXYZRGBA> *openni_viewer;

};

#endif
