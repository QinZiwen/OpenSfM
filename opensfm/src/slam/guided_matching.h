#pragma once
#include <Eigen/Eigen>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <opencv2/core.hpp>
// #include "third_party/openvslam/feature/orb_params.h"

namespace slam
{
// class Frame;
// class Landmark;
// class KeyFrame;
// class BrownPerspectiveCamera;
// class SlamReconstruction;
struct GridParameters
{
    GridParameters(unsigned int grid_col_, unsigned int grid_rows,
                   float img_min_width, float img_min_height,
                   float img_max_width, float img_max_height,
                   float inv_cell_width, float inv_cell_height);
    const unsigned int grid_cols_, grid_rows_;
    const float img_min_width_, img_min_height_;
    const float img_max_width_, img_max_height_;
    const float inv_cell_width_, inv_cell_height_;

    bool in_grid(const Eigen::Vector2f& pt2D) const { return in_grid(pt2D[0], pt2D[1]); }
    bool in_grid(const float x, const float y) const
    {
        return img_min_width_ < x && img_max_width_ > x && img_min_height_ < y && img_max_height_ > y;  
    }
};

using CellIndices = std::vector<std::vector<std::vector<size_t>>>;
using MatchIndices = std::vector<std::pair<size_t, size_t>>;
class GuidedMatcher
{
public:
    static constexpr unsigned int HAMMING_DIST_THR_LOW{50};
    static constexpr unsigned int HAMMING_DIST_THR_HIGH{100};
    static constexpr unsigned int MAX_HAMMING_DIST{256};

    //! ORB特徴量間のハミング距離を計算する
    static inline unsigned int 
    compute_descriptor_distance_32(const cv::Mat& desc_1, const cv::Mat& desc_2)
    {
        // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel

        constexpr uint32_t mask_1 = 0x55555555U;
        constexpr uint32_t mask_2 = 0x33333333U;
        constexpr uint32_t mask_3 = 0x0F0F0F0FU;
        constexpr uint32_t mask_4 = 0x01010101U;

        const auto* pa = desc_1.ptr<uint32_t>();
        const auto* pb = desc_2.ptr<uint32_t>();

        unsigned int dist = 0;

        for (unsigned int i = 0; i < 8; ++i, ++pa, ++pb) {
            auto v = *pa ^*pb;
            v -= ((v >> 1) & mask_1);
            v = (v & mask_2) + ((v >> 2) & mask_2);
            dist += (((v + (v >> 4)) & mask_3) * mask_4) >> 24;
        }

        return dist;
    }
    // static std::unordered_map<KeyFrame*, float>
    // compute_optical_flow(const cslam::Frame& new_frame);
    // static std::pair<float, float>
    // compute_min_max_depth(const KeyFrame& new_frame);

    // GuidedMatcher(const GridParameters& grid_params, const map::Camera& camera, SlamReconstruction* map_db);
    const GridParameters& grid_params_;
    GuidedMatcher(const GridParameters& grid_params);
    // const map::Camera& camera_;
    // void assign_points_to_grid(const Eigen::MatrixXf& undist_keypts, CellIndices& keypt_indices_in_cells);
    // CellIndices assign_keypoints_to_grid(const Eigen::MatrixXf& undist_keypts);

    // void distribute_keypoints_to_grid_frame(cslam::Frame& frame);

    void DistributeUndistKeyptsToGrid(const std::vector<cv::KeyPoint>& undist_keypts,
                                      CellIndices& keypt_indices_in_cells) const;

    MatchIndices MatchKptsToKpts(const std::vector<cv::KeyPoint>& undist_keypts_1, const cv::Mat& descriptors_1,
                                 const std::vector<cv::KeyPoint>& undist_keypts_2, const cv::Mat& descriptors_2,
                                 const CellIndices& keypts_indices_in_cells_2,
                                 const Eigen::MatrixX2f& prevMatched, const size_t margin) const;
    
    std::vector<size_t> 
    GetKeypointsInCell(const std::vector<cv::KeyPoint>& undist_keypts,
                       const CellIndices& keypt_indices_in_cells,
                       const float ref_x, const float ref_y, const float margin,
                       const int min_level = -1, const int max_level = -1) const;

    /*MatchIndices 
    match_frame_to_frame(const cslam::Frame& frame1, const cslam::Frame& frame2,
                         const Eigen::MatrixX2f& prevMatched,
                         const size_t margin);

    MatchIndices
    match_keyframe_to_frame_exhaustive(const cslam::KeyFrame& frame1, const cslam::Frame& frame2, const size_t margin) const;
    MatchIndices
    match_frame_to_frame_exhaustive(const cslam::Frame& frame1, const cslam::Frame& frame2, const size_t margin) const;
    MatchIndices
    match_kpts_to_kpts_exhaustive(const std::vector<cv::KeyPoint>& kpts1, const cv::Mat& desc1,
                                  const std::vector<cv::KeyPoint>& kpts2, const cv::Mat& desc2,
                                  const size_t margin) const;

    // TODO: Think about the margin. Maybe make it dynamic depending on the depth of the feature!!
    size_t
    match_frame_and_landmarks(cslam::Frame& frame, std::vector<cslam::Landmark*>& local_landmarks, const float margin);
    // std::vector<cslam::Landmark*>
    // update_local_landmarks(const std::vector<cslam::KeyFrame*>& local_keyframes, const size_t curr_frm_id);

    // size_t
    std::vector<std::pair<size_t, size_t>>
    match_current_and_last_frame(cslam::Frame& curr_frm, const cslam::Frame& last_frm, const float margin);

    size_t search_local_landmarks(std::vector<Landmark*>& local_landmarks, Frame& curr_frm);
    bool can_observe(Landmark* lm, const Frame& frame, const float ray_cos_thr, Eigen::Vector2f& reproj, size_t& pred_scale_level) const;

    MatchIndices
    match_for_triangulation(const KeyFrame& kf1, const KeyFrame& kf2, const Eigen::Matrix3f& E_12) const;
    MatchIndices
    match_for_triangulation_exhaustive(const KeyFrame& kf1, const KeyFrame& kf2, const Eigen::Matrix3f& E_12) const;
    MatchIndices 
    match_for_triangulation_with_depth(const KeyFrame& kf1, const KeyFrame& kf2, const Eigen::Matrix3f& E_12, const float median_depth) const;

    MatchIndices
    match_for_triangulation_epipolar(const KeyFrame& kf1, const KeyFrame& kf2, const Eigen::Matrix3f& E_12, const float min_depth, const float max_depth, const bool traverse_with_depth, const float margin = 5) const;

    template<typename T> size_t 
    replace_duplication(KeyFrame* keyfrm, const T& landmarks_to_check, const float margin = 3.0) const;
    SlamReconstruction* map_db_;*/
};
};