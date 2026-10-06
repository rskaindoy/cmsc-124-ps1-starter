# JOINT ANALYSIS

## NO. 1

<answer>


## NO. 2

<answer>


## NO. 3

An alternative design could **remove the `order` array entirely and keep only the hash buckets**. In our C implementation, the `order` array costs one `char *` per key (8 bytes on a 64-bit machine) and up to 2x slack from capacity doubling. Python’s `dict` pays a similar price to ensure insertion order, while Java offers `LinkedHashMap` separately from `HashMap`, so this cost is a known trade-off. Dropping it would also remove the growth logic in `dt_map_put` and make `dt_map_remove` faster, since it would no longer scan and shift the order array, which is O(n). However, the map would **lose its insertion order**. Iteration would follow bucket order instead, which depends on `hash_key` and on `dt_map_put` pushing new entries to the front of each chain. For example, with keys [A, B, C], removing and reinserting B should give [A, C, B], but without the array, the order would be arbitrary. We could make `dt_map_key_at` search through the buckets and count to the index, but that is O(n) per call, so printing every key becomes O(n*n), and it still would not preserve the insertion order. 


Hence, we would **not** ship the design that drops the `order` array. The printer depends on insertion order for stable output, so dropping it breaks a requirement. The cost of keeping the one with the `order` array is small, with one pointer per key and O(n) removal. 


## NO. 4

**Access after release** and **an allocation left unreleased at the driver's final check** are both errors in memory management, but the former is generally more immediately harmful than the latter because the program touches memory it no longer owns. In C, this is **undefined behavior**, such as reading incorrect data, corrupting memory, or a program crash. For example, hypothetically, if our `dt_record_free()` freed a field value that was still owned by the environment, and the environment later tried to access that value, the program would be accessing released memory, which could result in an immediate error. In a long-running server, this can become especially problematic as the same bug is repeated across many requests, increasing the chance of **failed requests, corrupted data, or even server downtime.**


On the other hand, an allocation that remains unreleased is a **memory leak**. In our implementation, if we forget the `free()` calls, the memory would remain occupied and unusable. The program may continue running normally at first because a leak does not corrupt anything, but in a long-running server that allocates repeatedly, the leaks can accumulate and eventually cause **high memory usage or allocation failures.**


Hence, in a long-running server, both problems matter, but in different ways, as **access after release causes immediate errors or crashes, while leaks gradually consume available memory.** In a command-line tool that exits in a second, the difference is less noticeable because the operating system reclaims the process’s memory on exit, so **a memory leak does little harm**. **Access after release remains problematic**, however, since it can still cause incorrect behavior or a crash before the program exits. Leaks become severe only when the process stays alive for a long time.
