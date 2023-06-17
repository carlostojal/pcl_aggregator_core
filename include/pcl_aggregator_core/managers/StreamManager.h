//
// Created by carlostojal on 01-05-2023.
//

#ifndef PCL_AGGREGATOR_CORE_STREAMMANAGER_H
#define PCL_AGGREGATOR_CORE_STREAMMANAGER_H

#include <string>
#include <memory>
#include <set>
#include <queue>
#include <mutex>
#include <eigen3/Eigen/Dense>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/registration/icp.h>
#include <pcl_aggregator_core/entities/StampedPointCloud.h>
#include <pcl_aggregator_core/utils/Utils.h>
#include <thread>
#include <functional>

#define STREAM_ICP_MAX_CORRESPONDENCE_DISTANCE 1
#define STREAM_ICP_MAX_ITERATIONS 10

namespace pcl_aggregator {
    namespace managers {

        /*! \brief Manager of a stream of PointClouds.
         *
         * Manages a stream of PointClouds coming from a single sensor.
         * For example, merges and ages the PointClouds captured by a single LiDAR.
         *
         * */
        class StreamManager {

            private:
                std::string topicName;
                /*! \brief Shared pointer to the merged PointCloud generated by this manager. */
                std::shared_ptr<entities::StampedPointCloud> cloud = nullptr;
                /*! \brief Transform from the sensor frame to the robot base frame. */
                Eigen::Affine3d sensorTransform;
                /*! \brief Is the transform of the sensor to the robot frame set. */
                bool sensorTransformSet = false;

                /*! \brief A set containing the PointClouds managed by this class ordered and identified by timestamp. */
                std::set<std::shared_ptr<entities::StampedPointCloud>,
                    entities::CompareStampedPointCloudPointers> clouds;
                /*! \brief Queue of PointClouds waiting for the transform to be set. */
                std::queue<std::shared_ptr<entities::StampedPointCloud>> cloudsNotTransformed;
                /*! \brief Maximum age points live for. After this time they will be removed. */
                double maxAge;

                /*! \brief Mutex to manage access to the clouds set. */
                std::mutex setMutex;
                /*! \brief Mutex to manage access to the merged PointCloud. */
                std::mutex cloudMutex;
                /*! \brief Mutex to manage access to the sensor transform. */
                std::mutex sensorTransformMutex;

                /*! \brief Thread which monitors the current PointCloud's age. Started by the constructor. */
                std::thread maxAgeWatcherThread;
                /*!\brief Flag to determine if the age watcher thread should be stopped or not. */
                bool keepAgeWatcherAlive = true;

                /*! \brief Callback function to call when a PointCloud ages older than maxAge.
                 * May be useful to remove points from the PointCloudsManager's PointCloud.
                 *
                 * @param label The label to remove
                 */
                std::function<void(std::uint32_t label)> pointAgingCallback = nullptr;

                /*! \brief Compute the sensor transform. */
                void computeTransform();
                void removePointCloud(std::shared_ptr<entities::StampedPointCloud> spcl);

            public:
                StreamManager(const std::string& topicName, double maxAge);
                ~StreamManager();

                bool operator==(const StreamManager& other) const;


                /*!
                 * \brief Feed a PointCloud to manage.
                 * @param cloud The PointCloud smart pointer.
                 */
                void addCloud(const pcl::PointCloud<pcl::PointXYZRGBL>::Ptr& cloud);

                /*!
                 * \brief Get the merged version of the still valid PointClouds fed into this manager.
                 * @return The merged PointCloud smart pointer.
                 */
                pcl::PointCloud<pcl::PointXYZRGBL>::Ptr getCloud() const; // returning the pointer prevents massive memory copies

                /*!
                 * \brief Set the transform between the sensor frame and the robot base frame.
                 * @param transform
                 */
                void setSensorTransform(const Eigen::Affine3d& transform);

                /*!
                 * \brief Get the max age points live for after being fed.
                 * @return The configured max points age.
                 */
                double getMaxAge() const;

                /*! \brief Get the defined point aging callback.
                 *
                 * @return The defined point aging callback.
                 */
                std::function<void(std::uint32_t label)> getPointAgingCallback() const;

                /*! \brief Set the point aging callback.
                 *
                 * @param func The callback to set.
                 */
                void setPointAgingCallback(const std::function<void(std::uint32_t label)>& func);


            /*!
             * \brief PointCloud transform routine.
             * Method intended to be called from a thread to transform the StampedPointCloud of a StreamManager in detached state.
             *
             * @param instance The StreamManager instance pointer in which this PointCloud exists.
             * @param spcl The PointCloud shared pointer to transform.
             * @param tf The transform to apply in form of an affine transformation.
             */
            friend void applyTransformRoutine(StreamManager* instance,
                                              const std::shared_ptr<entities::StampedPointCloud>& spcl,
                                              const Eigen::Affine3d& tf);


            friend void pointCloudAutoRemoveRoutine(StreamManager* instance,
                                                    const std::shared_ptr<entities::StampedPointCloud>& spcl);


            friend void icpTransformPointCloudRoutine(const std::shared_ptr<entities::StampedPointCloud>& spcl,
                                                      const Eigen::Matrix4f& tf);

            /*! \brief Max age watching routine.
             *
             * This routine is ran by a thread in background watching the age of the PointClouds
             * contained in this StreamManager. Whenever a thread older than the max age is found it is removed.
             *
             * @param instance Pointer to the StreamManager instance
             */
            friend void maxAgeWatchingRoutine(StreamManager* instance);

        };

    } // pcl_aggregator
} // managers

#endif //PCL_AGGREGATOR_CORE_STREAMMANAGER_H
