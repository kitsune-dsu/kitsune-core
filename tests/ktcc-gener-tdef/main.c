
/*  Snort does this annoying typedef where they abstract away the pointer to "struct tSfPolicyUserContext"
  file: examples/snort/snort-2.9.2/src/sfutil/sfPolicyUserData.h:
  code: typedef tSfPolicyUserContext * tSfPolicyUserContextId;
  And then for whatever reason, they set this structure pointer equal to NULL...
  this somehow confuses xfgen and leaves a dangling "gen_t" in the dsu.c file as shown in this example.
  When the "NULL" is not present, it works fine.

  TODO: look into this.
*/

#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

struct _tSfPolicyUserContext
{
    /**policy id of configuration file or packet being processed.
    */
    int  currentPolicyId;

    /**Number of policies currently allocated.
     */
    unsigned int numAllocatedPolicies;

    /**Number of policies active. Since we use an array of policy pointers,
     * number of allocated policies may be more than active policies. */
    unsigned int numActivePolicies;

    /**user configuration for a policy. This is a pointer to an array of pointers
     * to user configuration.
    */
    void E_T(@t) **  E_PTRARRAY(self.numAllocatedPolicies) userConfig;

} E_GENERIC(@t);

typedef struct _tSfPolicyUserContext E_G(@t) tSfPolicyUserContext E_GENERIC(@t);

typedef tSfPolicyUserContext E_G(@t)*  tSfPolicyUserContextId E_GENERIC(@t);



typedef struct _POPConfig
{
    char  ports[12];
    uint32_t   memcap;
    int max_depth;
    int b64_depth;
    int qp_depth;
    int bitenc_depth;
    int uu_depth;

} POPConfig;




/* Using the typedef works just fine as long as it's not set to NULL....*/
tSfPolicyUserContextId E_G(POPConfig) pop_config_ID_nonull_works;

/*does not work - set to NULL*/
tSfPolicyUserContextId E_G(POPConfig) pop_config_ID_eqnull_broken = NULL;

/* Current solution in Snort example: removing the typedef and replacing 
  it with the original structure allows it to still be set to NULL*/
tSfPolicyUserContext E_G(POPConfig) *  pop_config_noID_eqnull_works = NULL;

int main(int argc, char **argv) 
{

  kitsune_update("main");
  return 0;
}
