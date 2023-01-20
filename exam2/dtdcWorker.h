/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLE1N1PROCESSOR_H_
#define FAIRMQEXAMPLE1N1PROCESSOR_H_

#include "FairMQDevice.h"

class Worker : public FairMQDevice
{
  public:
    Worker();
    virtual ~Worker();

  protected:
    void Init() override;
    bool HandleData(FairMQMessagePtr&, int);

    uint32_t fId;
};

#endif /* FAIRMQEXAMPLE1N1PROCESSOR_H_ */
