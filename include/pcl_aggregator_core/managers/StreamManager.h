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
#include <thread>

#define STREAM_ICP_MAX_CORRESPONDENCE_DISTANCE 1
#define STREAM_ICP_MAX_ITERATIONS 10

namespace pcl_aggregator {
    namespace managers {

        /*! \brief Manager of a stream of PointClouds.
         *
         * Manages a stream of PointClouds coming from a single sensor.
         * For example, merges and ages the PointClouds captured by a single LiDAR.
         *
         * @tparam LabeledPointTypeT For example, if the sensor this stream manages is a stereo camera use PointXYZRGBL, if it is a LiDAR with no reflectivity return use PointXYZL. It is mandatory to be a type with label, used internally to manage aging.
         * */
        template <typename LabeledPointTypeT>
        class StreamManager {

            private:
                std::string topicName;
                /*! \brief Shared pointer to the merged PointCloud generated by this manager. */
                std::shared_ptr<entities::StampedPointCloud<LabeledPointTypeT>> cloud = nullptr;
                /*! \brief Transform from the sensor frame to the robot base frame. */
                Eigen::Affine3d sensorTransform;
                /*! \brief Is the transform of the sensor to the robot frame set. */
                bool sensorTransformSet = false;

                /*! \brief A set containing the PointClouds managed by this class ordered and identified by timestamp. */
                std::set<std::shared_ptr<entities::StampedPointCloud<LabeledPointTypeT>>,
                    entities::CompareStampedPointCloudPointers<LabeledPointTypeT>> clouds;
                /*! \brief Queue of PointClouds waiting for the transform to be set. */
                std::queue<std::shared_ptr<entities::StampedPointCloud<LabeledPointTypeT>>> cloudsNotTransformed;
                /*! \brief Maximum age points live for. After this time they will be removed. */
                double maxAge;

                /*! \brief Mutex to manage access to the clouds set. */
                std::mutex setMutex;
                /*! \brief Mutex to manage access to the merged PointCloud. */
                std::mutex cloudMutex;
                /*! \brief Mutex to manage access to the sensor transform. */
                std::mutex sensorTransformMutex;

                /*! \brief Compute the sensor transform. */
                void computeTransform();
                void removePointCloud(std::shared_ptr<entities::StampedPointCloud<LabeledPointTypeT>> spcl);

            public:
                StreamManager(std::string topicName, double maxAge);
                ~StreamManager();

                bool operator==(const StreamManager& other) const;


                /*!
                 * \brief Feed a PointCloud to manage.
                 * @param cloud The PointCloud smart pointer.
                 */
                void addCloud(const typename pcl::PointCloud<LabeledPointTypeT>::Ptr& cloud);

                /*!
                 * \brief Get the merged version of the still valid PointClouds fed into this manager.
                 * @return The merged PointCloud smart pointer.
                 */
                pcl::PointCloud<LabeledPointTypeT>::Ptr getCloud() const; // returning the pointer prevents massive memory copies

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


            /*!
             * \brief PointCloud transform routine.
             * Method intended to be called from a thread to transform the StampedPointCloud of a StreamManager in detached state.
             *
             * @tparam RoutinePointTypeT The type of points involved.
             * @param instance The StreamManager instance pointer in which this PointCloud exists.
             * @param spcl The PointCloud shared pointer to transform.
             * @param tf The transform to apply in form of an affine transformation.
             */
            template <typename RoutinePointTypeT>
            friend void applyTransformRoutine(StreamManager* instance,
                                              const std::shared_ptr<entities::StampedPointCloud<RoutinePointTypeT>>& spcl,
                                              const Eigen::Affine3d& tf);

            /*!
             * \brief Points auto-remove routine after max age.
             * This method is called by a detached thread the moment a StampedPointCloud is created on the StreamManager, waits for the max age, and then removes the points.
             *
             * @tparam RoutinePointTypeT The type of points.
             * @param instance The StreamManager instance pointer.
             * @param spcl The shared pointer of the StampedPointCloud to manage.
             */
            template <typename RoutinePointTypeT>
            friend void pointCloudAutoRemoveRoutine(StreamManager* instance,
                                                    const std::shared_ptr<entities::StampedPointCloud<RoutinePointTypeT>>& spcl);

            /*!
             * \brief ICP transform routine.
             * This method is called from a thread to transform the pointcloud in detached state.
             *
             * @tparam RoutinePointTypeT The type of points.
             * @param spcl The StamedPointCloud smart pointer to transform.
             * @param tf The ICP transform to apply.
             */
            template <typename RoutinePointTypeT>
            friend void icpTransformPointCloudRoutine(const std::shared_ptr<entities::StampedPointCloud<RoutinePointTypeT>>& spcl,
                                                      const Eigen::Matrix4f& tf);

        };

    } // pcl_aggregator
} // managers

#endif //PCL_AGGREGATOR_CORE_STREAMMANAGER_H