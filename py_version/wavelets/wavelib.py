import pywt

class WaveletCompression:
    """
    A class for wavelet compression.
    Input: 
        signal: 1D array
        level: level of wavelet decomposition
        wavelet: wavelet type
        compression_rate: compression rate
    Output:
        reconstructed compressed_signal: 1D array
    """
    def __init__(self, rate, wavelet='haar', level=5) -> None:
        self.rate = rate
        self.wavelet = wavelet
        self.level = level
        pass
    
    def compress_2d(self, signal : list) -> list:
        """
        Compress the signal.
        """
        tim_series, val_series = self.delta_encoding(signal)
        
        coeffs_time = pywt.wavedec(tim_series, self.wavelet, level=self.level)
        coeffs_valu = pywt.wavedec(val_series, self.wavelet, level=self.level)
        
        K = int(len(tim_series) * self.rate) - len(coeffs_time[0]) 
        print("K = {}".format(K))
        self.coeffs_topk(coeffs_time, K)
        self.coeffs_topk(coeffs_valu, K)
        
        reconstructed_tim_series = pywt.waverec(coeffs_time, self.wavelet)
        reconstructed_val_series = pywt.waverec(coeffs_valu, self.wavelet)
        
        new_signal = self.delta_decoding(reconstructed_tim_series, reconstructed_val_series)
        return new_signal
    
    def compress_1d(self, signal : list, is_level_topk = False) -> list:
        """
        Compress the signal.
        """
        coeffs = pywt.wavedec(signal, self.wavelet, level=self.level)
        
        # print(len(coeffs[0]))
        
        # count non-zeros in signal
        non_zeros = 0
        for val in signal:
            if val != 0:
                non_zeros += 1
        # print(f"non_zeros = {non_zeros}")
        K = int(non_zeros * self.rate) - len(coeffs[0]) 
        print("K = {}".format(K))
        if K < 0:
            K = 0
        if is_level_topk:
            self.coeffs_level_topk(coeffs, K)
        else:
            self.coeffs_topk(coeffs, K)
        reconstructed = pywt.waverec(coeffs, self.wavelet)

        return reconstructed
    
    def delta_encoding(self, signal : list) -> (list, list):
        """
        Delta coding, transfer the signal to time series and value series.
        For example, [0, 0, 0, 1, 0, 0, 0, 0, 1, 0] -> [3, 1], [6, 1]
        """
        tim_series = []
        val_series = []
        last_idx = 0
        for idx, val in enumerate(signal):
            if val == 0:
                continue
            tim_series.append(idx - last_idx)
            val_series.append(val)
            last_idx = idx
        return tim_series, val_series
    
    def delta_decoding(self, signal_1 : list, signal_2 : list) -> list:
        """
        Delta coding, transfer time series and value series to one signal
        For example,  [3, 5], [6, 1] -> [0, 0, 0, 1, 0, 0, 0, 0, 1] 
        """
        signal = [0] * int(sum(signal_1))
        last_time = 0
        missing = 0
        for idx, time in enumerate(signal_1):
            if last_time + round(time) < len(signal):
                signal[last_time + round(time)] = signal_2[idx]
            else:
                signal.append(signal_2[idx])
            last_time = last_time + round(time)
        return signal
        
    
    def coeffs_topk(self, all_coeffs : list, k) -> None:
        """
        Get top k coefficients.
        """
        topk = {}
        for level_id, level_coeffs in enumerate(all_coeffs):
            if level_id == 0:
                # reserve all approximation coefficients
                continue
            for coeff_id, coeff in enumerate(level_coeffs):
                if len(topk) < k:
                    topk[(level_id, coeff_id)] = coeff
                else:
                    # evict the smallest one
                    smallest_idx = (1, 0)
                    smallest_val = float('inf')
                    for (id1, id2) in topk.keys():
                        if (1.414**id1) * abs(topk[(id1, id2)]) < (1.414**smallest_idx[0]) * abs(smallest_val):
                            smallest_idx = id1, id2
                            smallest_val = topk[(id1, id2)]
                    if abs(coeff) > abs(smallest_val):
                        topk.pop(smallest_idx)
                        topk[(level_id, coeff_id)] = coeff
                pass
        pass
        # reserve the top k coefficients, set other coefficients to 0
        for level_id, level_coeffs in enumerate(all_coeffs):
            if level_id == 0:
                # reserve all approximation coefficients
                continue
            for coeff_id, _ in enumerate(level_coeffs):
                if (level_id, coeff_id) in topk.keys():
                    continue
                else:
                    all_coeffs[level_id][coeff_id] = 0
            pass
        pass
    
    def coeffs_level_topk(self, all_coeffs : list, k) -> None:
        """
        Get top k coefficients.
        """
        levels_topk = [{} for _ in range(self.level + 1)]
        total = sum([len(x) if level != 0 else 0 for level, x in enumerate(all_coeffs)])
        for level_id, level_coeffs in enumerate(all_coeffs):
            if level_id == 0:
                # reserve all approximation coefficients
                continue
            kk = int(k * len(level_coeffs) / total) + 1
            topk = levels_topk[level_id]
            for coeff_id, coeff in enumerate(level_coeffs):
                if len(topk) < kk:
                    topk[(level_id, coeff_id)] = coeff
                else:
                    # evict the smallest one
                    smallest_idx = None
                    smallest_val = float('inf')
                    for (id1, id2) in topk.keys():
                        if abs(topk[(id1, id2)]) < abs(smallest_val):
                            smallest_idx = id1, id2
                            smallest_val = topk[(id1, id2)]
                    if abs(coeff) > abs(smallest_val):
                        topk.pop(smallest_idx)
                        topk[(level_id, coeff_id)] = coeff
                pass
        pass
        # reserve the top k coefficients, set other coefficients to 0
        for level_id, level_coeffs in enumerate(all_coeffs):
            if level_id == 0:
                # reserve all approximation coefficients
                continue
            topk = levels_topk[level_id]
            for coeff_id, _ in enumerate(level_coeffs):
                if (level_id, coeff_id) in topk.keys():
                    continue
                else:
                    all_coeffs[level_id][coeff_id] = 0
            pass
        pass
                    