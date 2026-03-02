struct COOMatrix {
    unsigned int numRows;
    unsigned int numCols;
    unsigned int numNonZeros;
    unsigned int* rowIdxs;
    unsigned int* colIdxs;
    float* values;
};

void spmv_coo_gpu(COOMatrix cooMatrix, float* inVector, float* outVector);
