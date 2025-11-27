// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#ifndef CLomacorMapsFilter_H_
#define CLomacorMapsFilter_H_

#include "CyC_TYPES.h"
#include <CCycFilterBase.h>
#include "env/COcTreeUtils.h"
#include "CZenodo.h"

class CLomacorMapsFilter : public CCycFilterBase
{
public:
	explicit CLomacorMapsFilter(CycDatablockKey key);
	explicit CLomacorMapsFilter(const ConfigFilterParameters& params);

	~CLomacorMapsFilter() override;

	bool enable() override;
	bool disable() override;

private:
    bool process() override;
    void loadFromDatastream(const std::string& datastream_entry, const std::string& db_root_path) override;

	static std::vector<std::pair<CyC_INT, std::string>> decode(const std::vector<CyC_INT>& _maps_metadata);

private:
	CCycFilterBase* m_pInputFilterMapsServer = nullptr;
	CyC_TIME_UNIT   m_lastTsServer = 0;

	CCycFilterBase* m_pInputFilterSlam = nullptr;
	CyC_TIME_UNIT   m_lastTsSlam = 0;

	CZenodo			m_Zenodo;
	std::string		m_sCity;
};

#endif /* CLomacorMapsFilter_H_ */
