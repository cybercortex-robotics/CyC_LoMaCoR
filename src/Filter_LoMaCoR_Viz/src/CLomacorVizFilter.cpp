// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CLomacorVizFilter.h"
#include "vision/CImageDisplayUtils.h"
#include <msgpack.hpp>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>

DYNALO_EXPORT CyC_FILTER_TYPE DYNALO_CALL getFilterType()
{
	CycDatablockKey key;
	return CLomacorVizFilter(key).getFilterType();
}

DYNALO_EXPORT CCycFilterBase* DYNALO_CALL createFilter(const ConfigFilterParameters _params)
{
	return new CLomacorVizFilter(_params);
}

#define UNPACK(var) { \
	msgpack::unpack(handle, data, length, offset); \
	var = handle.get().as<typename std::decay<decltype(var)>::type>(); \
}

CPose unpack_pose(msgpack::object_handle& handle, char* data, long& length, size_t& offset)
{
	CyC_INT id;
	float tx, ty, tz;
	float rx, ry, rz, rw;   // rotation as quaternion

	UNPACK(id);
	UNPACK(tx);
	UNPACK(ty);
	UNPACK(tz);
	UNPACK(rx);
	UNPACK(ry);
	UNPACK(rz);
	UNPACK(rw);

	return CPose(tx, ty, tz, rx, ry, rz, rw, id);
}

CycPoint unpack_obs(msgpack::object_handle& handle, char* data, long& length, size_t& offset)
{
	CycPoint pt;

	UNPACK(pt.id);
	UNPACK(pt.key.nCoreID);
	UNPACK(pt.key.nFilterID);
	UNPACK(pt.pt2d.x());
	UNPACK(pt.pt2d.y());
	UNPACK(pt.score);
	UNPACK(pt.depth);
	UNPACK(pt.angle);

	// Unpack descriptor
	CyC_INT len; UNPACK(len);
	CyC_INT type; UNPACK(type);
	pt.descriptor = cv::Mat(1, len, type);
	for (CyC_INT i = 0; i < len; ++i)
	{
		if (type == CV_8UC1)
		{
			unsigned char d; UNPACK(d);
			pt.descriptor.at<unsigned char>(i) = d;
		}
		else if (type == CV_32F)
		{
			float d; UNPACK(d);
			pt.descriptor.at<float>(i) = d;
		}
		else
		{
			spdlog::error("CMapStorage: Unsupported descriptor type");
		}
	}

	return pt;
}

CycVoxel unpack_map_point(msgpack::object_handle& handle, char* data, long& length, size_t& offset)
{
	CyC_LONG nCreationKF;
	UNPACK(nCreationKF);

	CycVoxel vx;
	UNPACK(vx.id);
	UNPACK(vx.pt3d.x());
	UNPACK(vx.pt3d.y());
	UNPACK(vx.pt3d.z());
	UNPACK(vx.pt3d.w());
	UNPACK(vx.error);

	Eigen::Vector3f m_Normal;
	UNPACK(m_Normal.x());
	UNPACK(m_Normal.y());
	UNPACK(m_Normal.z());

	// Unpack map point visibility
	CyC_UINT num_visibility; UNPACK(num_visibility);
	for (CyC_UINT i = 0; i < num_visibility; ++i)
	{
		CyC_LONG frame_id; UNPACK(frame_id);
		CyC_LONG obs_id; UNPACK(obs_id);
	}

	return vx;
}

CLomacorVizFilter::CLomacorVizFilter(CycDatablockKey key) : CCycFilterBase(key)
{
	// Assign the filter type, input type and output type
	setFilterType("CyC_LOMACOR_VIZ_FILTER_TYPE");
	m_OutputDataType = CyC_IMAGE;
}

CLomacorVizFilter::CLomacorVizFilter(const ConfigFilterParameters& params) : CCycFilterBase(params)
{
	// Assign the filter type, input type and output type
	setFilterType("CyC_LOMACOR_VIZ_FILTER_TYPE");
	m_OutputDataType = CyC_IMAGE;
}

CLomacorVizFilter::~CLomacorVizFilter()
{
	if (m_bIsEnabled)
		disable();
}

bool CLomacorVizFilter::enable()
{
	// Define image size for displaying, based on the sensors model
	m_SizeImgDisp.width = 300u;
	m_SizeImgDisp.height = 200u;
	
	if (!this->isReplayFilter() && !this->isNetworkFilter())
	{
		// Read input filters
		for (CycInputSource input : this->getInputSources())
		{
			if (input.pCycFilter->getFilterType() == CStringUtils::CyC_HashFunc("CyC_LOMACOR_SERVER_FILTER_TYPE"))
				m_pInputServerFilter = input.pCycFilter;
			else if (input.pCycFilter->getFilterType() == CStringUtils::CyC_HashFunc("CyC_LOMACOR_MAPS_FILTER_TYPE"))
				m_pInputMapsFilters.push_back(input.pCycFilter);
		}

		if (m_pInputServerFilter == nullptr && m_pInputMapsFilters.size() == 0)
		{
			spdlog::info("Filter [{}-{}]: CLomacorVizFilter::enable(): No input sources. Disabling the LoMaCoR visualization filter.", getFilterKey().nCoreID, getFilterKey().nFilterID);
			return false;
		}
	}

	// Get display images width and height
	if (!m_CustomParameters["width"].empty())
		m_SizeImgDisp.width = std::stoi(m_CustomParameters["width"]);
	if (!m_CustomParameters["height"].empty())
		m_SizeImgDisp.height = std::stoi(m_CustomParameters["height"]);

    m_bIsEnabled = true;
	return true;
}

bool CLomacorVizFilter::disable()
{
	if (isRunning())
        stop();

	m_bIsEnabled = false;
	return true;
}

bool CLomacorVizFilter::process()
{
	bool bReturn(false);
	
	if (m_pInputServerFilter != nullptr)
	{
		CyC_TIME_UNIT readInputTsServer = m_pInputServerFilter->getTimestampStop();
		if (readInputTsServer > m_lastTsServer)
		{
			std::vector<CyC_INT> maps_metadata;
			if (m_pInputServerFilter->getData(maps_metadata))
			{
				m_lastTsServer = readInputTsServer;
				m_MapsMetadata = decode(maps_metadata);

				for (const auto& map_meta : m_MapsMetadata)
				{
					bool bFound = false;
					if (auto it = m_Maps.find(map_meta.first) != m_Maps.end())
						bFound = true;

					if (!bFound)
						m_Maps[map_meta.first] = read_map_data("C:/dev/src/CyberCortex.AI/CyC_Slam/etc/env/maps/exploratory.map");
				}
			}
		}
	}

	cv::Mat img = cv::Mat::zeros(m_SizeImgDisp * 5, CV_8UC3);

	// Create zones
	int offset = 10;
	
	if (!check_map_exists(1, m_MapsMetadata))
		add_dark_area(img, cv::Rect(offset, offset, (img.cols / 2.) - offset, (img.rows / 2.) - offset));
	else
	{
		cv::Mat disp_birds_view;
		float bew_scale = 30.f; // 150.f; // 50.f; // 3.5f;
		cv::Size grid_size(600, 360);
		CImageDisplayUtils::draw_slam(disp_birds_view, grid_size, m_Maps[1], bew_scale, true, false, false);
		cv::Rect rect(offset, offset, (img.cols / 2.) - offset, (img.rows / 2.) - offset);
		cv::Mat disp_rect_info = img(rect);
		cv::resize(disp_birds_view, disp_birds_view, rect.size());
		cv::addWeighted(disp_birds_view, 1., disp_rect_info, 0., 0., disp_rect_info);
	}
	if (!check_map_exists(2, m_MapsMetadata))
		add_dark_area(img, cv::Rect((img.cols / 2.) + offset, offset, (img.cols / 2.) - offset * 2, (img.rows / 2.) - offset));
	if (!check_map_exists(3, m_MapsMetadata))
		add_dark_area(img, cv::Rect(offset, (img.rows / 2.) + offset, (img.cols / 2.) - offset, (img.rows / 2.) - offset * 2));
	if (!check_map_exists(4, m_MapsMetadata))
		add_dark_area(img, cv::Rect((img.cols / 2.) + offset, (img.rows / 2.) + offset, (img.cols / 2.) - offset * 2, (img.rows / 2.) - offset * 2));

	cv::putText(img, "Zone 1", cv::Point(offset * 2, (img.rows / 2.) - offset * 2), cv::FONT_HERSHEY_PLAIN, 2, color::red, 3);
	cv::putText(img, "Zone 2", cv::Point(img.cols - 120 - offset * 2, (img.rows / 2.) - offset * 2), cv::FONT_HERSHEY_PLAIN, 2, color::yellow, 3);
	cv::putText(img, "Zone 3", cv::Point(offset * 2, (img.rows / 2.) + offset * 4), cv::FONT_HERSHEY_PLAIN, 2, color::green, 3);
	cv::putText(img, "Zone 4", cv::Point(img.cols - 120 - offset * 2, (img.rows / 2.) + offset * 4), cv::FONT_HERSHEY_PLAIN, 2, color::blue, 3);

	bReturn = true;

	if (bReturn)
	{
		CycImages dst;
		dst.emplace_back(img);
		updateData(dst);
	}

	return bReturn;
}

void CLomacorVizFilter::loadFromDatastream(const std::string& datastream_entry, const std::string& db_root_path)
{}

bool CLomacorVizFilter::check_map_exists(const CyC_INT& _id, const std::vector<std::pair<CyC_INT, std::string>>& _maps)
{
	bool bFound = false;
	for (const auto& map : _maps)
		if (map.first == _id)
			bFound = true;
	return bFound;
}

CycSlam CLomacorVizFilter::read_map_data(const std::string& _map_file)
{
	CycSlam slam_data;

	if (!CFileUtils::FileExist(_map_file.c_str()))
	{
		spdlog::warn("Map file '{}' does not exist.", _map_file);
		return slam_data;
	}
	spdlog::info("Reading map file '{}'", _map_file);

	msgpack::sbuffer buf;
	msgpack::packer packer(buf);
	std::ifstream infile(_map_file, std::ifstream::binary);

	// Get size of file and allocate memory
	infile.seekg(0, infile.end);
	long length = infile.tellg();
	infile.seekg(0);
	char* data = new char[length];

	// Read content
	infile.read(data, length);
	infile.close();

	msgpack::object_handle handle; // do not touch
	size_t offset = 0; // do not touch

	// Unpack frames
	int mps_counter; UNPACK(mps_counter);
	CyC_UINT num_frames; UNPACK(num_frames);
	for (CyC_UINT k = 0; k < num_frames; ++k)
	{
		UNPACK(slam_data.id);
		UNPACK(slam_data.timestamp);
		UNPACK(slam_data.is_keyframe);
		slam_data.Absolute_Body_W = unpack_pose(handle, data, length, offset);
		slam_data.Absolute_Cam_C = unpack_pose(handle, data, length, offset);
		slam_data.Absolute_Imu_W = unpack_pose(handle, data, length, offset);

		slam_data.prev_poses_Body_W.emplace_back(std::make_pair(slam_data.id, slam_data.Absolute_Body_W));
		slam_data.prev_poses_type.emplace_back(1);

		// Unpack observations
		CyC_UINT num_observations; UNPACK(num_observations);
		for (size_t i = 0; i < num_observations; ++i)
			CycPoint pt = unpack_obs(handle, data, length, offset);

		// Unpack map points in frame
		CyC_UINT num_map_point_in_frame; UNPACK(num_map_point_in_frame);
		for (size_t i = 0; i < num_map_point_in_frame; ++i)
		{
			CyC_INT id; UNPACK(id);
		}
	}

	// Unpack map points
	CyC_UINT num_map_points; UNPACK(num_map_points);
	for (CyC_UINT i = 0; i < num_map_points; ++i)
	{
		CycVoxel vx_W = unpack_map_point(handle, data, length, offset);
		slam_data.rel_map_points_W.emplace_back(std::make_pair(vx_W, CycPoint()));
	}

	delete[] data;

	return slam_data;
}

std::vector<std::pair<CyC_INT, std::string>> CLomacorVizFilter::decode(const std::vector<CyC_INT>& _maps_metadata)
{
	std::vector<std::pair<CyC_INT, std::string>> maps;

	CyC_INT cr = static_cast<CyC_INT>('\n');
	bool bIsId = false;
	CyC_INT decoded_id;
	std::vector<CyC_INT> decoded_link;

	// Deserialize: Extract id and link from maps_metadata
	for (const auto& q : _maps_metadata)
	{
		if (q == cr)
		{
			if (!decoded_link.empty())
			{
				std::string sLink;
				for (CyC_INT val : decoded_link)
					sLink += static_cast<char>(val);
				maps.emplace_back(std::make_pair(decoded_id, sLink));
			}

			bIsId = true;
			continue;
		}

		if (bIsId)
		{
			decoded_id = q;
			decoded_link.clear();
			bIsId = false;
			continue;
		}

		decoded_link.emplace_back(q);
	}

	if (!decoded_link.empty())
	{
		std::string sLink;
		for (CyC_INT val : decoded_link)
			sLink += static_cast<char>(val);
		maps.emplace_back(std::make_pair(decoded_id, sLink));
	}

	return maps;
}

void CLomacorVizFilter::add_dark_area(cv::Mat& _img, const cv::Rect& _rect)
{
	double alpha = 0.5; double beta = 0.5; double gamma = 0.;

	cv::Mat disp_rect_info = _img(_rect);
	cv::Mat black_rect = cv::Mat::zeros(disp_rect_info.rows, disp_rect_info.cols, CV_8UC3);
	cv::addWeighted(black_rect, alpha, disp_rect_info, beta, gamma, disp_rect_info);
}
