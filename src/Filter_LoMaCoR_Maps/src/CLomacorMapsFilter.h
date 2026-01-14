// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#ifndef CLomacorMapsFilter_H_
#define CLomacorMapsFilter_H_

#include "CyC_TYPES.h"
#include <CCycFilterBase.h>
#include "env/COcTreeUtils.h"
#include "CStateMachine.h"

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

private:
	CCycFilterBase* m_pInputFilterMapsServer = nullptr;
	CyC_TIME_UNIT   m_lastTsServer = 0;

	std::unique_ptr<CStateMachine> m_pStateMachine = nullptr;
	fs::path		m_MapsFolder;
	std::string		m_sRegion;
	unsigned int	m_nMapID = 0;
};

#endif /* CLomacorMapsFilter_H_ */
