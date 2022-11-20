#include <vector>
#include <fbxsdk.h>

#define MAX_NUM_BONES_PER_VERTEX 4

class FBXSkeleton {
public:
	FBXSkeleton();

	bool Load(FbxMesh* mesh);

private:
    struct VertexBoneData
    {
        unsigned int BoneIDs[MAX_NUM_BONES_PER_VERTEX] = { 0 };
        float Weights[MAX_NUM_BONES_PER_VERTEX] = { 0.0f };

        VertexBoneData()
        {
        }

        void AddBoneData(unsigned int BoneID, float Weight)
        {
            for (unsigned int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
                //if ((BoneIDs[i] == BoneID) && (Weights[i] != 0.0)) { // ‚·‚Å‚ÉŠŽ‚µ‚Ä‚¢‚½‚ç’Ç‰Á‚µ‚È‚¢
                //    return;
                //}
                if (Weights[i] == 0.0) {
                    BoneIDs[i] = BoneID;
                    Weights[i] = Weight;
                    //printf("Adding bone %d weight %f at index %i\n", BoneID, Weight, i);
                    return;
                }
            }

            // should never get here - more bones than we have space for
            //assert(0);
        }
    };

    std::vector<VertexBoneData> mBones;
};