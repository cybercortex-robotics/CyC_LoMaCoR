// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#ifndef CLomacorVizFilter_H_
#define CLomacorVizFilter_H_

#include "CyC_TYPES.h"
#include "CCycFilterBase.h"

class CLomacorVizFilter : public CCycFilterBase
{
public:
	explicit CLomacorVizFilter(CycDatablockKey key);
	explicit CLomacorVizFilter(const ConfigFilterParameters& params);

	~CLomacorVizFilter() override;

	bool enable() override;
	bool disable() override;

private:
    bool process() override;
    void loadFromDatastream(const std::string& datastream_entry, const std::string& db_root_path) override;

	bool check_map_exists(const CyC_INT& _id, const std::vector<std::pair<CyC_INT, std::string>>& _maps);
	CycSlam read_map_data(const std::string& _map_file);

	void add_dark_area(cv::Mat& _img, const cv::Rect& _rect);
	static std::vector<std::pair<CyC_INT, std::string>> decode(const std::vector<CyC_INT>& _maps_metadata);

private:
	CCycFilterBase*					m_pInputServerFilter;
	CyC_TIME_UNIT					m_lastTsServer = 0;
	std::vector<CCycFilterBase*>	m_pInputMapsFilters;

	std::vector<std::pair<CyC_INT, std::string>> m_MapsMetadata;
	std::unordered_map<CyC_INT, CycSlam> m_Maps;

	// Visualization parameters
	cv::Size	m_SizeImgDisp;
};

#endif /* CLomacorVizFilter_H_ */
