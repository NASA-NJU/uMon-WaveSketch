from math import sqrt, log2, floor, ceil

HEAVY_WAVE_LEVEL = 8
HEAVY_WAVE_K = 64
LIGHT_WAVE_LEVEL = 8
LIGHT_WAVE_K = 64
REPORT_WINDOW = 2048 # 1024 * 10us ~= 10ms
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
               
class heavy_entry:
    def __init__(self) -> None:
        self.key = None
        self.counter = wavetlet_counter(HEAVY_WAVE_K, REPORT_WINDOW, HEAVY_WAVE_LEVEL)
        self.vote = 0
    
    def __sizeof__(self):
        return 13 + wavetlet_counter(HEAVY_WAVE_K, REPORT_WINDOW, HEAVY_WAVE_LEVEL).__sizeof__() + 4

    # define setter and getter for key, counter, vote
    
class uSketch:
    def __init__(self, memory_size, h_w = 3, ratio=0.5, time_window=TIME_WINDOW) -> None:
        self.global_min_time = 0x7fffffff
        self.time_window = time_window
        self.heavy_memory = memory_size * 1024 * ratio
        self.heavy_w = h_w
        self.heavy_d = int(self.heavy_memory // h_w // heavy_entry().__sizeof__())
        self.heavy_part = [
            [heavy_entry() for _ in range(int(self.heavy_w))] for _ in range(self.heavy_d)
        ]
        self.light_memory = memory_size * 1024 * (1 - ratio)
        self.light_d = int(self.light_memory // wavetlet_counter(LIGHT_WAVE_K, REPORT_WINDOW, LIGHT_WAVE_LEVEL).__sizeof__())
        self.light_part = [
            wavetlet_counter(LIGHT_WAVE_K, REPORT_WINDOW, LIGHT_WAVE_LEVEL) for _ in range(self.light_d)
        ]
        print(f"h_w = {h_w}, ratio = {ratio}, heavy_d = {self.heavy_d}, light_d = {self.light_d}")
        pass
    
    def update(self, f, time_ns, byte):
        # print(f"f {f}, time {time_ns} wid = {wid}")
        hash_row_hv = hash(f) % self.heavy_d
        hash_row_lt = hash(f) % self.light_d
        self.light_part[hash_row_lt].update(time_ns, byte)
        for hf in self.heavy_part[hash_row_hv]:
            if hf.key == f:
                hf.counter.update(time_ns, byte)
                hf.vote += 1
                break
            elif hf.key is None:
                hf.key = f
                hf.counter.update(time_ns, byte)
                hf.vote += 1
                break
            else:
                hf.vote -= 1
                if hf.vote < 0:
                    hf.key = f
                    hf.vote = -hf.vote
                    hf.counter = wavetlet_counter(HEAVY_WAVE_K, REPORT_WINDOW, HEAVY_WAVE_LEVEL)
                    hf.counter.update(time_ns, byte)
                    break
        self.global_min_time = min(self.global_min_time, time_ns)
            
    def reconstruct(self):
        for row in self.heavy_part:
            for hf in row:
                hf.counter.reconstruct()
        for row in self.light_part:
            row.reconstruct()
            
        for row in self.heavy_part:
            for hf in row:
                hash_row_lt = hash(hf.key) % self.light_d
                for off, c in enumerate(hf.counter.result):
                    temp_wid = hf.counter.init_wid + off
                    if c > 0 and temp_wid < self.global_min_time // self.time_window + REPORT_WINDOW:
                        light_off = temp_wid- self.light_part[hash_row_lt].init_wid
                        # print(f"heavy {hf.key} wid {temp_wid} {c}, in light widoff {light_off} {self.light_part[hash_row_lt].result[light_off] } .. ")
                        self.light_part[hash_row_lt].result[light_off] -= c
                        if self.light_part[hash_row_lt].result[light_off]  < 0:
                            self.light_part[hash_row_lt].result[light_off] = 0
                    pass
                pass
            pass
        pass
    
    def is_heavy_wid(self, f, wid):
        """_summary_

        Args:
            f (_type_): _description_
            wid (_type_): _description_
        """
        hash_row_hv = hash(f) % self.heavy_d
        is_heavy = False
        for hf in self.heavy_part[hash_row_hv]:
            if hf.key == f:
                re = hf.counter.query(wid)
                is_heavy = True
        if is_heavy is False or re < 0:
            is_heavy = False
        return is_heavy

    def is_heavy_flow(self, f):
        """_summary_

        Args:
            f (_type_): _description_
            wid (_type_): _description_
        """
        hash_row_hv = hash(f) % self.heavy_d
        is_heavy = False
        for hf in self.heavy_part[hash_row_hv]:
            if hf.key == f:
                is_heavy = True
        return is_heavy
            
    def query(self, f, wid):
        hash_row_hv = hash(f) % self.heavy_d
        re = 0
        is_heavy = False
        for hf in self.heavy_part[hash_row_hv]:
            if hf.key == f:
                re = hf.counter.query(wid)
                is_heavy = True
        if is_heavy is False or re == -1:
            hash_row_lt = hash(f) % self.light_d
            re = self.light_part[hash_row_lt].query(wid)     
        if re < 0:
            re = 0
        return re      
                
    def get_heavy_flows(self):
        flows = {}
        for row in self.heavy_part:
            for hf in row:
                if hf.key is not None:
                    flows[hf.key] = hf.counter.result
        return flows
    
    def get_light_flows(self):
        flows = {}
        for id in range(len(self.light_part)):
            flows[id] = self.light_part[id].result
        return flows
    
if __name__ == "__main__":
    usketch = uSketch(200, 4, 0.5)
    