#include "log.h"
#include "sinoscope.h"

int sinoscope_opencl_init(sinoscope_opencl_t* opencl, cl_device_id opencl_device_id, unsigned int width,
                          unsigned int height) {
    /*
     * TODO
     *
     * Initialiser `opencl->context` du périphérique reçu en paramètre.
     *
     * Initialiser `opencl->queue` à partir du contexte précèdent.
     *
     * Initialiser `opencl->buffer` à partir du context précèdent (width * height * 3).
     *
     * Initialiser `opencl->kernel` à partir du contexte et du périphérique reçu en
     * paramètre. Utilisez la fonction `opencl_load_kernel_code` déclaré dans `opencl.h` pour lire
     * le code du noyau OpenCL `kernel/sinoscope.cl` dans un tampon. Compiler le noyau
     * en ajoutant le paramètre `"-I " __OPENCL_INCLUDE__`.
     */

    return -1;
}

void sinoscope_opencl_cleanup(sinoscope_opencl_t* opencl) {
    /*
     * TODO
     *
     * Libérez les ressources associées à `opencl->context`, `opencl->queue`,
     * `opencl->buffer` et `opencl->kernel`.
     */
}

/*
 * TODO
 *
 * Déclarez les structures partagées avec le noyau OpenCL si nécessaire selon votre énoncé.
 * Utilisez l'attribut `__attribute__((packed))` à vos déclarations.
 */

int sinoscope_image_opencl(sinoscope_t* sinoscope) {
    if (sinoscope->opencl == NULL) {
        LOG_ERROR_NULL_PTR();
        goto fail_exit;
    }

    /*
     * TODO
     *
     * Configurez les paramètres du noyau OpenCL afin d'envoyer `sinoscope->opencl->buffer` et les
     * autres données nécessaire à son exécution.
     *
     * Démarrez le noyau OpenCL et attendez son exécution.
     *
     * Effectuez la lecture du tampon `sinoscope->opencl->buffer` contenant le résultat dans
     * `sinoscope->buffer`.
     */

fail_exit:
    return -1;
}
