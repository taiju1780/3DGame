//////////////////////////////////////////////////////////////////
// full.fx ver1.2+�@�ݒ�p�����[�^v1.0�@by�J���e�K�@2018/2/26   //
//////////////////////////////////////////////////////////////////
//���v���O�������́u//�v�����̍ŏ��ɕt���Ă���ꏊ�ȊO�ł͑S�Ĕ��p�p������
//�@�L�����ĉ������I�I�@���ɃX�y�[�X�␔����S�p�ŏ����ԈႢ�͔������₷���ł��I

//���m�[�}���}�b�v�֌W/////////////////////////////////////////////////////////
//���m�[�}���}�b�v�Ƃ��Ďg�p����e�N�X�`���̖������w�肵�܂��B�i���g�p�������ގ����ł͂���܂���j
//�P�O���܂ŉ�(0�`10)
#define MAKE_NORMAL_TEX_COUNT 10

//���m�[�}���}�b�v�̏ꏊ�ƃm�[�}���}�b�v���g�p�������ގ��̎w��
//�w��̎d��(���ԈႦ�₷���̂ŕK���ҏW�O�Ƀt�@�C�����o�b�N�A�b�v���Ă���ҏW���I)
//�͂��߂ɏ�Ŏw�肵���e�N�X�`���̖������A�u�g�p����ގ��ԍ��v�̍��ڂ�
//�擪����u//�v���O���܂��B
//����Ŏw�肵���������A�e�s�擪�́u//�v�������s�̐�����v���Ă��Ȃ��ƃG���[�ɂȂ�܂��B
//���Ɏg�p����ގ��ԍ����u""�v�ň͂�ꂽ���ɔ��p�����ŋL�����A
//���̍ގ��Ŏg�p�������e�N�X�`���̏ꏊ(�t�@�C���p�X)���L�����܂��B
//���̍ގ��ԍ��ƃe�N�X�`���̏ꏊ�̋L������Ŏw�肵���e�N�X�`���̖������A�L�����ĉ������B
//�i���m�[�}���}�b�v���w�肵�Ȃ������ގ���full.fx�Ƃ���MMD�W���ɋ߂��ގ��ɂȂ�܂��j
//1����
#define USE_NORMAL_MAP_MATERIALS0 "17"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS0 "2b_skirt_01e_n.png"    //�e�N�X�`���̏ꏊ

//2����
#define USE_NORMAL_MAP_MATERIALS1 "9"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS1 "2b_chest_01_n.png"    //�e�N�X�`���̏ꏊ

//3����
#define USE_NORMAL_MAP_MATERIALS2 "10"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS2 "2b_arm_00_n.png"    //�e�N�X�`���̏ꏊ

//4����
#define USE_NORMAL_MAP_MATERIALS3 "22"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS3 "2b_hair_01_n.png"    //�e�N�X�`���̏ꏊ

//5����
#define USE_NORMAL_MAP_MATERIALS4 "20"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS4 "2b_Sashbelt_01_n.png"    //�e�N�X�`���̏ꏊ

//6����
#define USE_NORMAL_MAP_MATERIALS5 "15"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS5 "2b_thigh_high_boots_01_n.png"    //�e�N�X�`���̏ꏊ

//7����
#define USE_NORMAL_MAP_MATERIALS6 "25"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS6 "2b_front_01a_n.png"    //�e�N�X�`���̏ꏊ

//8����
#define USE_NORMAL_MAP_MATERIALS7 "24"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS7 "2b_back_03_n.png"    //�e�N�X�`���̏ꏊ

//9����
#define USE_NORMAL_MAP_MATERIALS8 "8"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS8 "2b_leotard_02_n.png"    //�e�N�X�`���̏ꏊ

//10����
#define USE_NORMAL_MAP_MATERIALS9 "18"         //�g�p����ގ��ԍ�
#define MAP_PASS_MATERIALS9 "2b_skirt_01e_n.png"    //�e�N�X�`���̏ꏊ

//���e�N�X�`���t�B���^�ꊇ�ݒ�////////////////////////////////////////////////////////////////////
//���ٕ������t�B���^�i�Ȃ߂炩�\���������Ă��Y��@��TEXFILTER_MAXANISOTROPY���ݒ肵�܂��傤�j
//#define TEXFILTER_SETTING ANISOTROPIC
//�o�C���j�A�i�Ȃ߂炩�\���F�W���j
#define TEXFILTER_SETTING LINEAR
//�j�A���X�g�l�C�o�[�i��������\���F�e�N�X�`���̃h�b�g���Y��ɏo��j
//#define TEXFILTER_SETTING POINT

//���ٕ������t�B���^���g���Ƃ��̔{��
//1:Off or 2�̔{���Ŏw��(�傫���قǂȂ߂炩���ʁE���ב�)
//�Ή����Ă��邩�ǂ����́A�n�[�h�E�F�A����iRadeonHD�@��Ȃ�16�܂ł͑Ή����Ă���͂�
#define TEXFILTER_MAXANISOTROPY 16

//�ٕ������t�B���^�ɂ���
//�e�N�X�`����Miplevels��������x����Ƃ���Y��ɂȂ�܂��B
//���e�N�X�`���ǂݍ��݂�MME�ŏ����K�v������܂�
//2�̗ݏ�T�C�Y�̃e�N�X�`���ł���Ɣ�r�I�Â��n�[�h�E�F�A�ɂ��D�����ł��B

//���e�N�X�`���A�h���b�V���O�ꊇ�ݒ�//////////////////////////////////////////////////////////////
//�e�N�X�`���ʂɐݒ肵�����Ƃ��́A�T���v���[�X�e�[�g�L�q���ōs���܂��傤�B
//��U�����[�v
#define TEXADDRESS_U WRAP 
//��U���~���[�����[�v
//#define TEXADDRESS_U MIRROR
//��U��0to1�����FMMD�W��
//#define TEXADDRESS_U CLAMP
//��U��-1to1�����F-�����̓~���[�����O
//#define TEXADDRESS_U MIRRORONCE

//��V�����[�v
#define TEXADDRESS_V WRAP 
//��V���~���[�����[�v
//#define TEXADDRESS_V MIRROR
//��V��0to1�����FMMD�W��
//#define TEXADDRESS_V CLAMP
//��V��-1to1�����F-�����̓~���[�����O
//#define TEXADDRESS_V MIRRORONCE

//���ufull(NormalMap)_CORE.fx�v�̃t�@�C���p�X(�ꏊ)���L��//////////////////////////////////////////
#include "full(NormalMap)_CORE.fx"