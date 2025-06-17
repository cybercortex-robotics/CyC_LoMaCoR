// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CLomacorVizFilter.h"
#include "vision/CImageDisplayUtils.h"

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
				m_Maps = decode(maps_metadata);
			}
		}
	}

	cv::Mat img;
	CImageDisplayUtils::draw_slam_grid(img, m_SizeImgDisp);
	int offset = 10;

	// Create zones
	cv::putText(img, "Zone 1", cv::Point(offset * 2, (img.rows / 2.) - offset * 2), cv::FONT_HERSHEY_PLAIN, 2, color::red, 3);
	cv::putText(img, "Zone 2", cv::Point(img.cols - 120 - offset * 2, (img.rows / 2.) - offset * 2), cv::FONT_HERSHEY_PLAIN, 2, color::yellow, 3);
	cv::putText(img, "Zone 3", cv::Point(offset * 2, (img.rows / 2.) + offset * 4), cv::FONT_HERSHEY_PLAIN, 2, color::green, 3);
	cv::putText(img, "Zone 4", cv::Point(img.cols - 120 - offset * 2, (img.rows / 2.) + offset * 4), cv::FONT_HERSHEY_PLAIN, 2, color::blue, 3);

	if (!check_map_exists(1, m_Maps))
		add_dark_area(img, cv::Rect(offset, offset, (img.cols / 2.) - offset, (img.rows / 2.) - offset));
	if (!check_map_exists(2, m_Maps))
		add_dark_area(img, cv::Rect((img.cols / 2.) + offset, offset, (img.cols / 2.) - offset * 2, (img.rows / 2.) - offset));
	if (!check_map_exists(3, m_Maps))
		add_dark_area(img, cv::Rect(offset, (img.rows / 2.) + offset, (img.cols / 2.) - offset, (img.rows / 2.) - offset * 2));
	if (!check_map_exists(4, m_Maps))
		add_dark_area(img, cv::Rect((img.cols / 2.) + offset, (img.rows / 2.) + offset, (img.cols / 2.) - offset * 2, (img.rows / 2.) - offset * 2));

	//spdlog::info("Maps:");
	//for (const auto& q : m_Maps)
	//	spdlog::info("\t{}:{}", q.first, q.second);

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
