#pragma once

#include "m-ulator/architectures.h"
#include "m-ulator/instruction_decoder.h"
#include "m-ulator/memory_region.h"
#include "m-ulator/registers.h"
#include "m-ulator/types.h"

#include <functional>
#include <map>
#include <tuple>
#include <vector>

namespace mulator
{

    struct CPU_State
    {
        u32 registers[REGISTER_COUNT];

        struct
        {
            bool N;
            bool Z;
            bool C;
            bool V;
            bool Q;
            u8 it_state;
        } psr;

        u32 exclusive_address;

        u32 CCR;
        u8 PRIMASK;
        u8 FAULTMASK;
        u8 BASEPRI;
        u8 CONTROL;

        u32 time;
    };

    class Emulator
    {
    public:
        Emulator(Architecture arch);
        Emulator(const Emulator& other);
        ~Emulator();

        /*
         * Getters for internals.
         */
        InstructionDecoder get_decoder() const;
        Architecture get_architecture() const;

        /*
         * Sets the flash region.
         *
         * @param[in] offset - the starting address
         * @param[in] size - the memory region size
         */
        void set_flash_region(u32 offset, u32 size);

        /*
         * Getters for the flash region.
         */
        u32 get_flash_offset() const;
        u32 get_flash_size() const;
        u8* get_flash() const;

        /*
         * Sets the ram region.
         *
         * @param[in] offset - the starting address
         * @param[in] size - the memory region size
         */
        void set_ram_region(u32 offset, u32 size);

        /*
         * Getters for the ram region.
         */
        u32 get_ram_offset() const;
        u32 get_ram_size() const;
        u8* get_ram() const;

        /*
         * Read and write access to registers.
         * Does not fire any hooks!
         */
        u32 read_register(Register reg) const;
        void write_register(Register reg, u32 value);

        /*
         * Read and write access to memory.
         * Does not fire any hooks!
         */
        void read_memory(u32 address, u8* buffer, u32 len) const;
        void write_memory(u32 dst_address, const u8* buffer, u32 len);

        /*
         * Starts/continues emulation of the code from the current state.
         * Stops automatically after a maximum number of instructions or when the end address is hit.
         */
        ReturnCode emulate(u64 max_instructions);
        ReturnCode emulate(u32 end_address, u64 max_instructions);

        /*
         * Stop the ongoing emulation. Useful in hooks.
         */
        void stop_emulation();

        /*
         * Check whether the emulator is still emulating. Useful in hooks.
         */
        bool is_running() const;

        /*
         * Gets the total number of executed instructions in this emulator.
         */
        u32 get_time() const;

        /*
         * Gets the number of executed instructions in the last 'emulate' call.
         */
        u32 get_emulated_time() const;

        /*
         * Returns true if emulation is currently in an IT block.
         */
        bool in_IT_block() const;

        /*
         * Returns true if emulation is currently at the last instruction in an IT block.
         */
        bool last_in_IT_block() const;

        /*
         * Returns the current CPU state.
         * By backing up the CPU state and RAM contents the state of the emulator can be backed up completely.
         */
        CPU_State get_cpu_state() const;

        /*
         * Set the emulator to a specific CPU state.
         */
        void set_cpu_state(const CPU_State& state);

        /*
         * #####################################################################################
         * Hooks
         * #####################################################################################
         * Every "add_XY_hook" function returns a unique identifier that can be used to remove a hook.
         * This unique identifier is never 0.
         */

        /*
         * Hooking before instruction fetch, i.e., before memory is read and passed to the decoder.
         *
         * The hook callback receives:
         * emu - the emulator that executed the hook
         * address - the address of the instruction which will be fetched
         * instr_size - the size of the instruction which will be fetched
         * user_data - a pointer supplied while registering the hook
         */
        using hook_on_fetch_ptr = void (*)(Emulator& emu, u32 address, u32 instr_size, void* user_data);
        u32 add_before_fetch_hook(hook_on_fetch_ptr func, void* user_data = nullptr);
        using hook_on_fetch_func = std::function<void(Emulator& emu, u32 address, u32 instr_size, void* user_data)>;
        u32 add_before_fetch_hook(const hook_on_fetch_func& func, void* user_data = nullptr);

        /*
         * Hooking either after an instruction has been decoded or after it has been executed.
         *
         * The hook callback receives:
         * emu - the emulator that executed the hook
         * instruction - the decoded/executed instruction
         * user_data - a pointer supplied while registering the hook
         */
        using hook_instruction_ptr = void (*)(Emulator& emu, const Instruction& instruction, void* user_data);
        u32 add_instruction_decoded_hook(hook_instruction_ptr func, void* user_data = nullptr);
        u32 add_instruction_executed_hook(hook_instruction_ptr func, void* user_data = nullptr);
        using hook_instruction_func = std::function<void(Emulator& emu, const Instruction& instruction, void* user_data)>;
        u32 add_instruction_decoded_hook(const hook_instruction_func& func, void* user_data = nullptr);
        u32 add_instruction_executed_hook(const hook_instruction_func& func, void* user_data = nullptr);

        /*
         * Hooking memory access.
         *
         * The hook callback receives:
         * emu - the emulator that executed the hook
         * address - the address which is accessed
         * size - the number of bytes written, maximum 4
         * value - for the read hooks value holds the current content of the memory, for write hooks value holds the content to be written
         * user_data - a pointer supplied while registering the hook
         */
        using hook_on_memory_access_ptr = void (*)(Emulator& emu, u32 address, u32 size, u32 value, void* user_data);
        u32 add_memory_before_read_hook(hook_on_memory_access_ptr func, void* user_data = nullptr);
        u32 add_memory_after_read_hook(hook_on_memory_access_ptr func, void* user_data = nullptr);
        u32 add_memory_before_write_hook(hook_on_memory_access_ptr func, void* user_data = nullptr);
        u32 add_memory_after_write_hook(hook_on_memory_access_ptr func, void* user_data = nullptr);
        using hook_on_memory_access_func = std::function<void(Emulator& emu, u32 address, u32 size, u32 value, void* user_data)>;
        u32 add_memory_before_read_hook(const hook_on_memory_access_func& func, void* user_data = nullptr);
        u32 add_memory_after_read_hook(const hook_on_memory_access_func& func, void* user_data = nullptr);
        u32 add_memory_before_write_hook(const hook_on_memory_access_func& func, void* user_data = nullptr);
        u32 add_memory_after_write_hook(const hook_on_memory_access_func& func, void* user_data = nullptr);

        /*
         * Hooking register access.
         *
         * The hook callback receives:
         * emu - the emulator that executed the hook
         * reg - the regester which is accessed
         * value - for the read hooks value holds the current content of the register, for write hooks value holds the content to be written
         * user_data - a pointer supplied while registering the hook
         */
        using hook_on_register_access_ptr = void (*)(Emulator& emu, Register reg, u32 value, void* user_data);
        u32 add_register_before_read_hook(hook_on_register_access_ptr func, void* user_data = nullptr);
        u32 add_register_after_read_hook(hook_on_register_access_ptr func, void* user_data = nullptr);
        u32 add_register_before_write_hook(hook_on_register_access_ptr func, void* user_data = nullptr);
        u32 add_register_after_write_hook(hook_on_register_access_ptr func, void* user_data = nullptr);
        using hook_on_register_access_func = std::function<void(Emulator& emu, Register reg, u32 value, void* user_data)>;
        u32 add_register_before_read_hook(const hook_on_register_access_func& func, void* user_data = nullptr);
        u32 add_register_after_read_hook(const hook_on_register_access_func& func, void* user_data = nullptr);
        u32 add_register_before_write_hook(const hook_on_register_access_func& func, void* user_data = nullptr);
        u32 add_register_after_write_hook(const hook_on_register_access_func& func, void* user_data = nullptr);

        /*
         * Remove a specific hook by its id.
         */
        void remove_hook(u32 id);

        /*
         * Remove all registered hooks.
         */
        void clear_hooks();

    private:
        InstructionDecoder m_decoder;

        bool m_has_on_fetch_hooks;
        bool m_has_on_decode_hooks;
        bool m_has_on_execute_hooks;
        bool m_has_before_memory_read_hooks;
        bool m_has_after_memory_read_hooks;
        bool m_has_before_memory_write_hooks;
        bool m_has_after_memory_write_hooks;
        bool m_has_before_register_read_hooks;
        bool m_has_after_register_read_hooks;
        bool m_has_before_register_write_hooks;
        bool m_has_after_register_write_hooks;
        std::vector<std::tuple<u32, hook_on_fetch_ptr, hook_on_fetch_func, void*>> m_on_fetch_hooks;
        std::vector<std::tuple<u32, hook_instruction_ptr, hook_instruction_func, void*>> m_on_decode_hooks;
        std::vector<std::tuple<u32, hook_instruction_ptr, hook_instruction_func, void*>> m_on_execute_hooks;
        std::vector<std::tuple<u32, hook_on_memory_access_ptr, hook_on_memory_access_func, void*>> m_before_memory_read_hooks;
        std::vector<std::tuple<u32, hook_on_memory_access_ptr, hook_on_memory_access_func, void*>> m_after_memory_read_hooks;
        std::vector<std::tuple<u32, hook_on_memory_access_ptr, hook_on_memory_access_func, void*>> m_before_memory_write_hooks;
        std::vector<std::tuple<u32, hook_on_memory_access_ptr, hook_on_memory_access_func, void*>> m_after_memory_write_hooks;
        std::vector<std::tuple<u32, hook_on_register_access_ptr, hook_on_register_access_func, void*>> m_before_register_read_hooks;
        std::vector<std::tuple<u32, hook_on_register_access_ptr, hook_on_register_access_func, void*>> m_after_register_read_hooks;
        std::vector<std::tuple<u32, hook_on_register_access_ptr, hook_on_register_access_func, void*>> m_before_register_write_hooks;
        std::vector<std::tuple<u32, hook_on_register_access_ptr, hook_on_register_access_func, void*>> m_after_register_write_hooks;

        std::vector<u32> m_remove_hooks;
        std::unordered_map<u32, u8> m_hook_id_type;
        u32 m_next_hook_id;

        ReturnCode m_return_code;

        MemoryRegion m_flash;
        MemoryRegion m_ram;

        CPU_State m_cpu_state;
        u32 m_emulated_time;
        bool m_psr_updated;

        bool m_cleanup_requested;
        void cleanup_hooks();
        void cleanup_hook(u32 id);

        u8* validate_address(u32 address);
        void clock_cpu();

        u32 read_memory_internal(u32 address, u8 bytes);
        void write_memory_internal(u32 address, u32 value, u8 bytes);

        u32 read_register_internal(Register reg);
        void write_register_internal(Register reg, u32 value);

        Condition pop_IT_condition();

        bool execute(const Instruction& instr);

        bool evaluate_condition(const Instruction& instr);

        void branch_write_PC(u32 address);
        void bx_write_PC(u32 address);
        void blx_write_PC(u32 address);

        void set_exclusive_monitors(u32 address, u32 align);
        bool exclusive_monitors_pass(u32 address, u32 align) const;

        bool in_priviledged_mode() const;
        i32 get_execution_priority() const;
    };

} // namespace mulator
