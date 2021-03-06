//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_6502TS_CORE_DELAY_QUEUE_MEMBER
#define TIA_6502TS_CORE_DELAY_QUEUE_MEMBER

#include "bspf.hxx"

class DelayQueueMember
{
  public:

    struct Entry {
      uInt8 address;
      uInt8 value;
    };

  public:

    DelayQueueMember(uInt8 size);

    DelayQueueMember(DelayQueueMember&&) = default;

    DelayQueueMember& operator=(DelayQueueMember&&) = default;

  public:

    void push(uInt8 address, uInt8 value);

    void remove(uInt8 address);

    vector<Entry>::const_iterator begin() const {
      return myEntries.begin();
    }

    vector<Entry>::const_iterator end() const {
      return (mySize < myEntries.size()) ? (myEntries.begin() + mySize) : myEntries.end();
    }

    void clear() {
      mySize = 0;
    }

  private:

    vector<Entry> myEntries;

    size_t mySize;

  private:
    DelayQueueMember() = delete;
    DelayQueueMember(const DelayQueueMember&) = delete;
    DelayQueueMember& operator=(const DelayQueueMember&) = delete;

};

#endif //  TIA_6502TS_CORE_DELAY_QUEUE_MEMBER
