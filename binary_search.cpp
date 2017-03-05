// 返回最后一个不大于 target 的位置

template <typename T>
int binary_search(T *A, int low, int high, T target)
{
    while (low < high) {
        int mid = (low + high) / 2;
        target < A[mid] ? (high = mid) : (low = mid + 1); 
    }   

    return --low;
}
