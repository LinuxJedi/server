# The time on ANALYSE FORMAT=JSON is rather variable

--replace_regex /("(r_total_time_ms|r_buffer_size|r_filling_time_ms)": )[^, \n]*/\1"REPLACED"/
