//
//  ReductionTf.cpp
//  MNNConverter
//
//  Created by MNN on 2019/01/31.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#include <string.h>
#include "TfUtils.hpp"
#include "tfOpConverter.hpp"

#include "graph.pb.h"

DECLARE_OP_CONVERTER(ReductionTf);

MNN::OpType ReductionTf::opType() {
    return MNN::OpType_Reduction;
}
MNN::OpParameter ReductionTf::type() {
    return MNN::OpParameter_ReductionParam;
}

void ReductionTf::run(MNN::OpT *dstOp, TmpNode *srcNode, TmpGraph *tempGraph) {
    auto reductionParam = new MNN::ReductionParamT;

    TmpNode *constNode = tempGraph->_getTmpNode(srcNode->inEdges[1]);
    DCHECK(constNode->opType == "Const") << "Reduction Should have one Const Node" << srcNode->opName;

    // reduction parameter
    tensorflow::AttrValue value;

    reductionParam->dType = MNN::DataType_DT_FLOAT;
    if (find_attr_value(srcNode->tfNode, "T", value)) {
        reductionParam->dType = (MNN::DataType)value.type();
    }

    reductionParam->keepDims = false;
    if (find_attr_value(srcNode->tfNode, "keep_dims", value)) {
        reductionParam->keepDims = value.b();
    }

    if (find_attr_value(constNode->tfNode, "value", value)) {
        const tensorflow::TensorProto &reductionIndices           = value.tensor();
        const tensorflow::TensorShapeProto &reductionIndicesShape = reductionIndices.tensor_shape();
        int dimSize                                               = 1;
        if (reductionIndicesShape.dim_size() > 0) {
            dimSize = reductionIndicesShape.dim(0).size();
        }
        reductionParam->dim.resize(dimSize);
        if (reductionIndices.int_val_size() > 0) {
            for (int i = 0; i < dimSize; ++i) {
                reductionParam->dim[i] = reductionIndices.int_val(i);
            }
        } else {
            DCHECK((MNN::DataType)reductionIndices.dtype() == MNN::DataType_DT_INT32);
            DCHECK(reductionIndices.tensor_content().size() > 0);
            const int *dimData = (int *)reductionIndices.tensor_content().c_str();
            for (int i = 0; i < dimSize; i++) {
                reductionParam->dim[i] = dimData[i];
            }
        }
    }

    // reduction operation
    if (srcNode->opType == "Mean") {
        reductionParam->operation = MNN::ReductionType_MEAN;
    } else if (srcNode->opType == "Max") {
        reductionParam->operation = MNN::ReductionType_MAXIMUM;
    } else if (srcNode->opType == "Min") {
        reductionParam->operation = MNN::ReductionType_MINIMUM;
    } else if (srcNode->opType == "Sum") {
        reductionParam->operation = MNN::ReductionType_SUM;
    } else if (srcNode->opType == "Prod") {
        reductionParam->operation = MNN::ReductionType_PROD;
    } else {
        DLOG(ERROR) << "MNN Converter Not "
                       "Supported!!! ===> "
                    << srcNode->opType;
    }

    reductionParam->coeff = 0.0f; // defalut

    dstOp->main.value = reductionParam;

    DCHECK(srcNode->inTensors.size() == 1) << "Reduction Input Node ERROR!!! ===> " << srcNode->opName;
}

REGISTER_CONVERTER(ReductionTf, Mean);
REGISTER_CONVERTER(ReductionTf, Max);
REGISTER_CONVERTER(ReductionTf, Min);
REGISTER_CONVERTER(ReductionTf, Sum);
REGISTER_CONVERTER(ReductionTf, Prod);
