// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#ifndef CLomacorServerFilter_H_
#define CLomacorServerFilter_H_

#include "CyC_TYPES.h"
#include <CCycFilterBase.h>
#include "env/COcTreeUtils.h"

class CLomacorServerFilter : public CCycFilterBase
{
public:
	explicit CLomacorServerFilter(CycDatablockKey key);
	explicit CLomacorServerFilter(const ConfigFilterParameters& params);

	~CLomacorServerFilter() override;

	bool enable() override;
	bool disable() override;

private:
    bool process() override;
    void loadFromDatastream(const std::string& datastream_entry, const std::string& db_root_path) override;

private:
	std::vector<CyC_INT>	m_MapsMetadata;
	std::string				m_sMapsPath;
	bool					m_bMapsRead = false;
};

#endif /* CLomacorServerFilter_H_ */
