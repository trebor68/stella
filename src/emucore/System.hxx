//============================================================================
//
// MM     MM  6666  555555  0000   2222
// MMMM MMMM 66  66 55     00  00 22  22
// MM MMM MM 66     55     00  00     22
// MM  M  MM 66666  55555  00  00  22222  --  "A 6502 Microprocessor Emulator"
// MM     MM 66  66     55 00  00 22
// MM     MM 66  66 55  55 00  00 22
// MM     MM  6666   5555   0000  222222
//
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SYSTEM_HXX
#define SYSTEM_HXX

class Device;
class M6502;
class M6532;
class TIA;
class NullDevice;

#include "bspf.hxx"
#include "Device.hxx"
#include "NullDev.hxx"
#include "Random.hxx"
#include "Serializable.hxx"

/**
  This class represents a system consisting of a 6502 microprocessor
  and a set of devices.  The devices are mapped into an addressing
  space of 2^n bytes (1 <= n <= 16).  The addressing space is broken
  into 2^m byte pages (1 <= m <= n), where a page is the smallest unit
  a device can use when installing itself in the system.

  In general the addressing space will be 8192 (2^13) bytes for a
  6507 based system and 65536 (2^16) bytes for a 6502 based system.

  @author  Bradford W. Mott
*/
class System : public Serializable
{
  public:
    /**
      Create a new system with an addressing space of 2^13 bytes and
      pages of 2^6 bytes.
    */
    System(const OSystem& osystem, M6502& m6502, M6532& m6532,
           TIA& mTIA, Cartridge& mCart);
    virtual ~System() = default;

    // Mask to apply to an address before accessing memory
    static constexpr uInt16 ADDRESS_MASK = (1 << 13) - 1;

    // Amount to shift an address by to determine what page it's on
    static constexpr uInt16 PAGE_SHIFT = 6;

    // Mask to apply to an address to obtain its page offset
    static constexpr uInt16 PAGE_MASK = (1 << PAGE_SHIFT) - 1;

    // Number of pages in the system
    static constexpr uInt16 NUM_PAGES = 1 << (13 - PAGE_SHIFT);

  public:
    /**
      Initialize system and all attached devices to known state.
    */
    void initialize();

    /**
      Reset the system cycle counter, the attached devices, and the
      attached processor of the system.

      @param autodetect  A hint to devices that the system is currently
                         in autodetect mode.  That is, the system is being
                         run to autodetect certain device settings before
                         actual emulation will begin.  Certain devices may
                         use this hint to act differently under those
                         circumstances.
    */
    void reset(bool autodetect = false);

  public:
    /**
      Answer the 6502 microprocessor attached to the system.  If a
      processor has not been attached calling this function will fail.

      @return The attached 6502 microprocessor
    */
    M6502& m6502() const { return myM6502; }

    /**
      Answer the 6532 processor attached to the system.  If a
      processor has not been attached calling this function will fail.

      @return The attached 6532 microprocessor
    */
    M6532& m6532() const { return myM6532; }

    /**
      Answer the TIA device attached to the system.

      @return The attached TIA device
    */
    TIA& tia() const { return myTIA; }

    /**
      Answer the random generator attached to the system.

      @return The random generator
    */
    Random& randGenerator() const { return myOSystem.random(); }

    /**
      Get the null device associated with the system.  Every system
      has a null device associated with it that's used by pages which
      aren't mapped to "real" devices.

      @return The null device associated with the system
    */
    const NullDevice& nullDevice() const { return myNullDevice; }

  public:
    /**
      Get the number of system cycles which have passed since the last
      time cycles were reset or the system was reset.

      @return The number of system cycles which have passed
    */
    uInt32 cycles() const { return myCycles; }

    /**
      Increment the system cycles by the specified number of cycles.

      @param amount The amount to add to the system cycles counter
    */
    void incrementCycles(uInt32 amount) { myCycles += amount; }

    /**
      Reset the system cycle count to zero.  The first thing that
      happens is that all devices are notified of the reset by invoking
      their systemCyclesReset method then the system cycle count is
      reset to zero.
    */
    void resetCycles();

    /**
      Answers whether the system is currently in device autodetect mode.
    */
    bool autodetectMode() const { return mySystemInAutodetect; }

  public:
    /**
      Get the current state of the data bus in the system.  The current
      state is the last data that was accessed by the system.

      @return  The data bus state
    */
    uInt8 getDataBusState() const { return myDataBusState; }

    /**
      Get the current state of the data bus in the system, taking into
      account that certain bits are in Z-state (undriven).  In those
      cases, the bits are floating, but will usually be the same as the
      last data bus value (the 'usually' is emulated by randomly driving
      certain bits high).

      However, some CMOS EPROM chips always drive Z-state bits high.
      This is emulated by hmask, which specifies to push a specific
      Z-state bit high.

      @param zmask  The bits which are in Z-state
      @param hmask  The bits which should always be driven high
      @return  The data bus state
    */
    uInt8 getDataBusState(uInt8 zmask, uInt8 hmask = 0x00)
    {
      // For the pins that are floating, randomly decide which are high or low
      // Otherwise, they're specifically driven high
      return (myDataBusState | (randGenerator().next() | hmask)) & zmask;
    }

    /**
      Get the byte at the specified address.  No masking of the
      address occurs before it's sent to the device mapped at
      the address.

      @param address  The address from which the value should be loaded
      @param flags    Indicates that this address has the given flags
                      for type of access (CODE, DATA, GFX, etc)

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address, uInt8 flags = 0);

    /**
      Change the byte at the specified address to the given value.
      No masking of the address occurs before it's sent to the device
      mapped at the address.

      This method sets the 'page dirty' if the write succeeds.  In the
      case of direct-access pokes, the write always succeeds.  Otherwise,
      if the device is handling the poke, we depend on its return value
      for this information.

      @param address  The address where the value should be stored
      @param value    The value to be stored at the address
    */
    void poke(uInt16 address, uInt8 value);

    /**
      Lock/unlock the data bus. When the bus is locked, peek() and
      poke() don't update the bus state. The bus should be unlocked
      while the CPU is running (normal emulation, or when the debugger
      is stepping/advancing). It should be locked while the debugger
      is active but not running the CPU. This is so the debugger can
      use System.peek() to examine memory/registers without changing
      the state of the system.
    */
    void lockDataBus()   { myDataBusLocked = true;  }
    void unlockDataBus() { myDataBusLocked = false; }

    /**
      Access and modify the disassembly type flags for the given
      address.  Note that while any flag can be used, the disassembly
      only really acts on CODE/GFX/PGFX/DATA/ROW.
    */
    uInt8 getAccessFlags(uInt16 address) const;
    void setAccessFlags(uInt16 address, uInt8 flags);

  public:
    /**
      Describes how a page can be accessed
    */
    enum PageAccessType {
      PA_READ      = 1 << 0,
      PA_WRITE     = 1 << 1,
      PA_READWRITE = PA_READ | PA_WRITE
    };

    /**
      Structure used to specify access methods for a page
    */
    struct PageAccess
    {
      /**
        Pointer to a block of memory or the null pointer.  The null pointer
        indicates that the device's peek method should be invoked for reads
        to this page, while other values are the base address of an array
        to directly access for reads to this page.
      */
      uInt8* directPeekBase;

      /**
        Pointer to a block of memory or the null pointer.  The null pointer
        indicates that the device's poke method should be invoked for writes
        to this page, while other values are the base address of an array
        to directly access for pokes to this page.
      */
      uInt8* directPokeBase;

      /**
        Pointer to a lookup table for marking an address as CODE.  A CODE
        section is defined as any address that appears in the program
        counter.  Currently, this is used by the debugger/disassembler to
        conclusively determine if a section of address space is CODE, even
        if the disassembler failed to mark it as such.
      */
      uInt8* codeAccessBase;

      /**
        Pointer to the device associated with this page or to the system's
        null device if the page hasn't been mapped to a device.
      */
      Device* device;

      /**
        The manner in which the pages are accessed by the system
        (READ, WRITE, READWRITE)
      */
      PageAccessType type;

      // Constructors
      PageAccess()
        : directPeekBase(nullptr),
          directPokeBase(nullptr),
          codeAccessBase(nullptr),
          device(nullptr),
          type(System::PA_READ) { }

      PageAccess(Device* dev, PageAccessType access)
        : directPeekBase(nullptr),
          directPokeBase(nullptr),
          codeAccessBase(nullptr),
          device(dev),
          type(access) { }
    };

    /**
      Set the page accessing method for the specified page.

      @param page The page accessing methods should be set for
      @param access The accessing methods to be used by the page
    */
    void setPageAccess(uInt16 page, const PageAccess& access) {
      myPageAccessTable[page] = access;
    }

    /**
      Get the page accessing method for the specified page.

      @param page The page to get accessing methods for
      @return The accessing methods used by the page
    */
    const PageAccess& getPageAccess(uInt16 page) const {
      return myPageAccessTable[page];
    }

    /**
      Get the page type for the given address.

      @param addr  The address contained in the page in questions
      @return  The type of page that contains the given address
    */
    System::PageAccessType getPageAccessType(uInt16 addr) const {
      return myPageAccessTable[(addr & ADDRESS_MASK) >> PAGE_SHIFT].type;
    }

    /**
      Mark the page containing this address as being dirty.

      @param addr  Determines the page that is dirty
    */
    void setDirtyPage(uInt16 addr) {
      myPageIsDirtyTable[(addr & ADDRESS_MASK) >> PAGE_SHIFT] = true;
    }

    /**
      Answer whether any pages in given range of addresses have been
      marked as dirty.

      @param start_addr The start address; determines the start page
      @param end_addr   The end address; determines the end page
    */
    bool isPageDirty(uInt16 start_addr, uInt16 end_addr) const;

    /**
      Mark all pages as clean (ie, turn off the dirty flag).
    */
    void clearDirtyPages();

    /**
      Save the current state of this system to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const override;

    /**
      Load the current state of this system from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in) override;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "System"; }

  private:
    const OSystem& myOSystem;

    // 6502 processor attached to the system
    M6502& myM6502;

    // 6532 processor attached to the system
    M6532& myM6532;

    // TIA device attached to the system
    TIA& myTIA;

    // Cartridge device attached to the system
    Cartridge& myCart;

    // Number of system cycles executed since the last reset
    uInt32 myCycles;

    // Null device to use for page which are not installed
    NullDevice myNullDevice;

    // The list of PageAccess structures
    PageAccess myPageAccessTable[NUM_PAGES];

    // The list of dirty pages
    bool myPageIsDirtyTable[NUM_PAGES];

    // The current state of the Data Bus
    uInt8 myDataBusState;

    // Whether or not peek() updates the data bus state. This
    // is true during normal emulation, and false when the
    // debugger is active.
    bool myDataBusLocked;

    // Whether autodetection is currently running (ie, the emulation
    // core is attempting to autodetect display settings, cart modes, etc)
    // Some parts of the codebase need to act differently in such a case
    bool mySystemInAutodetect;

  private:
    // Following constructors and assignment operators not supported
    System() = delete;
    System(const System&) = delete;
    System(System&&) = delete;
    System& operator=(const System&) = delete;
    System& operator=(System&&) = delete;
};

#endif
