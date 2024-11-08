from math import sqrt, log2, floor, ceil

LIGHT_WAVE_LEVEL = 8
LIGHT_WAVE_K = 64
REPORT_WINDOW = 4096 # 1024 * 10us ~= 10ms
TIME_WINDOW = 10000 # 10us

class wavetlet_counter:
    def __init__(self, k = 64, capacity = 2048, level = 3, time_window=TIME_WINDOW) -> None:
        """_summary_

        Args:
            memory_size (int): memory size in Kbytes
        """
        self.capacity = capacity    # largest capacity according to report_window // micro_window
        self.approx = [0] * (capacity // 2**level + 1)
        self.detail_val = [0] * level
        self.detail_idx = [0] * level
        self.k = k
        self.reserved_detail = [[0, 0, 0] for _ in range(k)]
        self.level = level
        
        ## Temp varaible to help reconstruction
        self.init_wid = -1
        self.time_window = time_window
        self.temp_off = 0
        self.temp_count = 0
        
        ## result : only store in the analyzer
        self.result = []

    def query(self, wid):
        wid_off = wid - self.init_wid
        if wid_off < 0 or wid_off >= self.capacity:
            # print(f"Error: wid_off {wid_off} wid {wid} init_wid {self.init_wid} capacity {self.capacity}")
            return -1
        return self.result[wid_off]
        
    def compress(self, level, offset, value):
        # find min in reserved_detail
        ex = (level, offset, value)
        if value == 0:
            return
        for idx, x in enumerate(self.reserved_detail):
            if (1/sqrt(2))**(x[0]+1) * abs(x[2]) < (1/sqrt(2))**(ex[0]+1) * abs(ex[2]):
                temp = [ex[0], ex[1], ex[2]]
                ex = (x[0], x[1], x[2])
                self.reserved_detail[idx] = temp

    def update(self, time_ns, byte):
        if self.init_wid == -1:
            self.init_wid = time_ns // self.time_window
            self.temp_off = 0   # offset
        off = time_ns // self.time_window - self.init_wid  # can be implemented as masking lowering bits
                                                           # the time_window should be set as 8192 ns, 16384 ns, etc
        if off == self.temp_off:
            self.temp_count += byte
        else:
            self.transform(self.temp_off, self.temp_count)
            self.temp_off = off
            self.temp_count = byte

    def transform(self, off, x):
        # Update approx index
        pos_a = off >> self.level
        if pos_a >= len(self.approx):
            print(f"Error: pos_a {pos_a} len {len(self.approx)}")
            return
        self.approx[pos_a] = self.approx[pos_a] + x
        # Update multi-level detail index
        for l in range(self.level):
            pos_d = off >> (l+1)
            if pos_d > self.detail_idx[l]:
                self.compress(l, self.detail_idx[l], self.detail_val[l])
                self.detail_val[l] = 0
                self.detail_idx[l] = pos_d
            sign_d = (off >> l) & 1
            if sign_d == 0:
                self.detail_val[l] = self.detail_val[l] + x
            else:
                self.detail_val[l] = self.detail_val[l] - x
            
    def reconstruct(self):
        # Process rest data
        if self.init_wid == -1:
            self.result = [0] * self.capacity
            return
        seqlen = self.temp_off 
        self.transform(self.temp_off, self.temp_count)
        # Padding data to 2**n
        if seqlen == 0:
            seqlen = 1
        max_level = floor(log2(seqlen))
        for i in range(self.temp_off+1, 2**(max_level+1)):
            if i >= self.capacity:
                print(f"Error: i {i} capacity {self.capacity}")
            self.transform(i, 0)
            
        # Process push rest coefs.
        for l in range(self.level):
            self.compress(l, self.detail_idx[l], self.detail_val[l])
        
        max_level = min(max_level, self.level - 1)
        # Start reconstructing
        for l in range(max_level, -1, -1):
            recons = []
            n = ceil(self.capacity / 2**(l + 1))
            for idx in range(n):
                if self.approx == []:
                    a = 0
                else:
                    a = self.approx.pop(0)
                has_detail = False
                for lel, off, val in self.reserved_detail:
                    if lel == l and idx == off:
                        if val > a:
                            val = a
                        if val < -a:
                            val = -a
                        has_detail = True
                        recons.append((a + val) // 2)
                        recons.append((a - val) // 2)          
                        break
                if has_detail is False:
                    recons.append(a // 2)
                    recons.append(a // 2)
            self.approx = recons
        self.result = self.approx + [0] * (self.capacity - len(self.approx))

    def __sizeof__(self) -> int:
        return  len(self.approx) * 4 + self.level * 4 * 2 + self.k * 8 + 4
               # 4 + 80 + 64 * 8 = 532
               
    
class uSketch:
    def __init__(self, memory_size, time_window=TIME_WINDOW) -> None:
        self.global_min_time = 0x7fffffff
        self.time_window = time_window
        self.light_memory = memory_size * 1024 
        self.light_d = int(self.light_memory // wavetlet_counter(LIGHT_WAVE_K, REPORT_WINDOW, LIGHT_WAVE_LEVEL).__sizeof__())
        self.light_part = [
            wavetlet_counter(LIGHT_WAVE_K, REPORT_WINDOW, LIGHT_WAVE_LEVEL) for _ in range(self.light_d)
        ]
        pass
    
    def update(self, f, time_ns, byte):
        # print(f"f {f}, time {time_ns} wid = {wid}")
        hash_row_lt = hash(f) % self.light_d
        self.light_part[hash_row_lt].update(time_ns, byte)
        self.global_min_time = min(self.global_min_time, time_ns)
            
    def reconstruct(self):
        for row in self.light_part:
            row.reconstruct()
            
    def query(self, f, wid):
        hash_row_lt = hash(f) % self.light_d
        re = self.light_part[hash_row_lt].query(wid)     
        return re      
                