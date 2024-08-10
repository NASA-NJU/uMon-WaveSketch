import numpy as np

def euclidean_distance(seq1, seq2):
    # 将序列转换为NumPy数组
    arr1 = np.array(seq1)
    arr2 = np.array(seq2)
    
    if len(arr1) < len(arr2):
        arr2 = arr2[:len(arr1)]
    else:
        arr1 = arr1[:len(arr2)]

    # 计算差值的平方和
    squared_diff = np.sum((arr1 - arr2) ** 2)

    # 计算平方和的平方根并返回结果
    distance = np.sqrt(squared_diff)
    return distance