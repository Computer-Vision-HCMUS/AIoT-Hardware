#pragma once

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual bool begin() = 0;
};
