// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_UNINITIALIZEDQUEUE_H
#define SST_CORE_UNINITIALIZEDQUEUE_H

#include <sst/core/activityQueue.h>

namespace SST {

/** Always uninitialized queue
 * @brief Used for debugging, and preventing accidentally sending messages
 * into an incorrect queue
 */
class UninitializedQueue : public ActivityQueue {
public:
    /** Create a new Queue
     * @param message - Message to print when something attempts to use this Queue
     */
    UninitializedQueue(std::string message);
    UninitializedQueue(); // Only used for serialization
    ~UninitializedQueue();

    bool empty() override;
    int size() override;
    void insert(Activity* activity) override;
    Activity* pop() override;
    Activity* front() override;


private:
    std::string message;

};

} //namespace SST

#endif // SST_CORE_UNINITIALIZEDQUEUE_H
