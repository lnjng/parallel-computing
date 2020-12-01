#include <assert.h>
#include <stddef.h>

#include "heatsim.h"
#include "log.h"

struct whp_params_t {
    unsigned int width;
    unsigned int height;
    unsigned int padding;
}
struct data_params_t {
    double data[];
}

void init_struct_grid(MPI_Type_struct* whp_params, MPI_Type_struct* data_params){
    // init les variables pour mpi_type_struct
    int nb_params = 3;
    int blocklengths[nb_params];
    MPI_int indices[nb_params];
    MPI_Datatype old_types[nb_params];
    // longueurs
    blocklengths[0] = 1;
    blocklengths[1] = 1;
    blocklengths[2] = 1;
    // types de base
    old_types[0] = MPI_INTEGER;
    old_types[1] = MPI_INTEGER;
    old_types[2] = MPI_INTEGER;
    // situation relative
    indices[0] = offsetof(whp_params_t, width);
    indices[1] = offsetof(whp_params_t, height);
    indices[2] = offsetof(whp_params_t, padding);

    MPI_Type_struct(nb_params, blocklengths, indices, old_types, &whp_params);

    // create mpi_contiguous first
    int len_dyn_array = grid->width_padded*grid->height_padded;
    MPI_Datatype dyn_array_type;
    MPI_Type_contiguous(len_dyn_array, MPI_DOUBLE, &dyn_array_type);
    // init les variables pour mpi_type_struct
    int nb_params = 1;
    int blocklengths[nb_params];
    MPI_int indices[nb_params];
    MPI_Datatype old_types[nb_params];
    // longueurs
    blocklengths[0] = 1;
    // types de base
    old_types[0] = dyn_array_type;
    // addresse de chaque param
    indices[0] = 0;
    MPI_Type_struct(nb_params, blocklengths, indices, old_types, &data_params);
}

int heatsim_init(heatsim_t* heatsim, unsigned int dim_x, unsigned int dim_y) {
    /*
     * TODO: Initialiser tous les membres de la structure `heatsim`.
     *       Le communicateur doit être périodique. Le communicateur
     *       cartésien est périodique en X et Y.
     */
    int err = MPI_SUCCESS;

    err |= MPI_Comm_size(MPI_COMM_WORLD, &heatsim->rank_count);
    err |= MPI_Comm_rank(MPI_COMM_WORLD, &heatsim->rank);

	if (dim_x * dim_y != heatsim->rank_count) {
		printf("Block size needs to be the same value as nb of rank count");
		goto fail_exit;
	}

    err |= MPI_Cart_create(MPI_COMM_WORLD, 2, [dim_x, dim_y], [1, 1], false, &heatsim->communicator);
    err |= MPI_Cart_shift(heatsim->communicator, 1, 1, &heatsim->rank_north_peer, &heatsim->rank_south_peer);
    err |= MPI_Cart_shift(heatsim->communicator, 0, 1, &heatsim->rank_west_peer, &heatsim->rank_east_peer);
    err |= MPI_Cart_coords(MPI_COMM_WORLD, heatsim->rank, 2, heatsim->coordinates);

    if (err != MPI_SUCCESS){
        prinf("Failed to initialize MPI");
        goto fail_exit;
    }

    return 1;

fail_exit:
    return -1;
}

int heatsim_send_grids(heatsim_t* heatsim, cart2d_t* cart) {
    /*
     * TODO: Envoyer toutes les `grid` aux autres rangs. Cette fonction
     *       est appelé pour le rang 0. Par exemple, si le rang 3 est à la
     *       coordonnée cartésienne (0, 2), alors on y envoit le `grid`
     *       à la position (0, 2) dans `cart`.
     *
     *       Il est recommandé d'envoyer les paramètres `width`, `height`
     *       et `padding` avant les données. De cette manière, le receveur
     *       peut allouer la structure avec `grid_create` directement.
     *
     *       Utilisez `cart2d_get_grid` pour obtenir la `grid` à une coordonnée.
     */

    int nb_msg_send = 2;

    int nb_requests = (rank_count-1)*nb_msg_send;

    //better to have pointers?
    MPI_Request reqs[nb_requests*nb_msg_send];
    MPI_Status stats[nb_requests*nb_msg_send];

    for (int i = 1; i < rank_count; i++) {
        int err = MPI_SUCCESS;
        int tag = 0;

        int coordinates[2]; 
        err = MPI_Cart_coords(heatsim->communicator, i, 2, coordinates);

        if (err != MPI_SUCCESS){
            prinf("Failed to get cart coords MPI");
            goto fail_exit;
        }
        grid_t* grid |= cart2d_get_grid(cart, coordinates[0], coordinates[1]);

/*
        // init les variables pour mpi_type_struct
        int nb_params = 3;
        int blocklengths[nb_params];
        MPI_int indices[nb_params];
        MPI_Datatype old_types[nb_params];
        // longueurs
        blocklengths[0] = 1;
        blocklengths[1] = 1;
        blocklengths[2] = 1;
        // types de base
        old_types[0] = MPI_INTEGER;
        old_types[1] = MPI_INTEGER;
        old_types[2] = MPI_INTEGER;
        // addresse de chaque param
        MPI_Address(&grid->width, &indices[0]);
        MPI_Address(&grid->height, &indices[1]);
        MPI_Address(&grid->padding, &indices[2]);
        // situation relative
        indices[2] = indices[2] - indices[0];
        indices[1] = indices[1] - indices[0];
        indices[0] = 0;

        MPI_Type_struct(nb_params, blocklengths, indices, old_types, &whp_params);
        MPI_Type_commit(&whp_params);

        // pour data
        // create mpi_contiguous first
        int len_dyn_array = grid->width_padded*grid->height_padded;
        MPI_Datatype dyn_array_type;
        MPI_Type_contiguous(len_dyn_array, MPI_DOUBLE, &dyn_array_type);
        // init les variables pour mpi_type_struct
        MPI_Type_struct data_params;
        int nb_params = 1;
        int blocklengths[nb_params];
        MPI_int indices[nb_params];
        MPI_Datatype old_types[nb_params];
        // longueurs
        blocklengths[0] = 1;
        // types de base
        old_types[0] = dyn_array_type;
        // addresse de chaque param
        indices[0] = 0;
        MPI_Type_struct(nb_params, blocklengths, indices, old_types, &data_params);
        MPI_Type_commit(&data_params);
*/

        MPI_Type_struct whp_params;
        MPI_Type_struct data_params;
        init_struct_grid(&whp_params, &data_params);

        struct whp_params_t whp;
        whp.width = grid->width;
        whp.height = grid->height;
        whp.padding = grid->padding;
        err |= MPI_Isend(&whp, 1, whp_params, i, tag, heatsim->communicator, &reqs[(i-1)*nb_msg_send]);
        // create custom struct for data 
        struct data_params_t* d = (data_params_t *)malloc(len_dyn_array*sizeof(double));
        for (int i = 0; i < len_dyn_array; i++) {
            d.data[i] = grid->data[i];
        }
        err |= MPI_Isend(d, 1, data_params, i, tag, heatsim->communicator, &reqs[(i-1)*nb_msg_send+1]);

        if (err != MPI_SUCCESS){
            prinf("Failed to send grid to other ranks");
            goto fail_exit;
        }
    }

    err = MPI_Waitall(nb_requests, reqs, stats);

    MPI_TYPE_FREE(&whp_params);
    MPI_TYPE_FREE(&data_params);

fail_exit:
    return -1;
}

grid_t* heatsim_receive_grid(heatsim_t* heatsim) {
    /*
     * TODO: Recevoir un `grid ` du rang 0. Il est important de noté que
     *       toutes les `grid` ne sont pas nécessairement de la même
     *       dimension (habituellement ±1 en largeur et hauteur). Utilisez
     *       la fonction `grid_create` pour allouer un `grid`.
     *
     *       Utilisez `grid_create` pour allouer le `grid` à retourner.
     */
    int err = MPI_SUCCESS;
    int tag = 0;
    
    MPI_Request reqwhp;
    MPI_Status statwhp;

/*
    // init les variables pour mpi_type_struct
    MPI_Type_struct whp_params;
    int nb_params = 3;
    int blocklengths[nb_params];
    MPI_int indices[nb_params];
    MPI_Datatype old_types[nb_params];
    // longueurs
    blocklengths[0] = 1;
    blocklengths[1] = 1;
    blocklengths[2] = 1;
    // types de base
    old_types[0] = MPI_INTEGER;
    old_types[1] = MPI_INTEGER;
    old_types[2] = MPI_INTEGER;
    // addresse de chaque param
    MPI_Address( &grid->width, &indices[0]);
    MPI_Address( &grid->height, &indices[1]);
    MPI_Address( &grid->padding, &indices[2]);
    // situation relative
    indices[2] = indices[2] - indices[0];
    indices[1] = indices[1] - indices[0];
    indices[0] = 0;
    MPI_Type_struct(nb_params, blocklengths, indices, old_types, &whp_params);
    MPI_Type_commit(&whp_params);

    // Pour data
    // create mpi_contiguous first
    int len_dyn_array = grid->width_padded*grid->height_padded;
    MPI_Datatype dyn_array_type;
    MPI_Type_contiguous(len_dyn_array, MPI_DOUBLE, &dyn_array_type);
    // init les variables pour mpi_type_struct
    MPI_Type_struct data_params;
    int nb_params = 1;
    int blocklengths[nb_params];
    MPI_int indices[nb_params];
    MPI_Datatype old_types[nb_params];
    blocklengths[0] = 1;
    old_types[0] = dyn_array_type;
    indices[0] = 0;
    MPI_Type_struct(nb_params, blocklengths, indices, old_types, &data_params);
    MPI_Type_commit(&data_params);
    */

    MPI_Type_struct whp_params;
    MPI_Type_struct data_params;
    init_struct_grid(&whp_params, &data_params);
    struct whp_params_t whp;
    err |= MPI_Irecv(&whp, 1, whp_params, 0, tag, heatsim->communicator, &reqwhp);

    err |= MPI_Wait(&reqwhp, &statwhp);

    if (err != MPI_SUCCESS){
        prinf("Failed to receive width, height and/or padding");
        goto fail_exit;
    }
    MPI_TYPE_FREE(&whp_params);

    struct whp_params_t whp;
    grid_t* grid = grid_create(whp.width, whp.height, whp.padding);

    // create params_data la struct avec les infos donnees auparavant

    // cree la struct MPI comme avant
    // recevoir le data dans une var init en tant que data_params_t
    MPI_Request req;
    MPI_Status stat;
    int len_dyn_array = grid->height_padded*grid->width_padded;
    struct data_params_t* d = (data_params_t *)malloc(len_dyn_array*sizeof(double));
    err |= MPI_Irecv(d->data, 1, data_params, 0, tag, heatsim->communicator, &req);
    err |= MPI_Wait(&req, &stat);
    if (err != MPI_SUCCESS){
        prinf("Failed to receive grid data");
        goto fail_exit;
    }
    MPI_TYPE_FREE(&data_params);

    return grid;

fail_exit:
    return NULL;
}

int heatsim_exchange_borders(heatsim_t* heatsim, grid_t* grid) {
    assert(grid->padding == 1);

    /*
     * TODO: Échange les bordures de `grid`, excluant le rembourrage, dans le
     *       rembourrage du voisin de ce rang. Par exemple, soit la `grid`
     *       4x4 suivante,
     *
     *                            +-------------+
     *                            | x x x x x x |
     *                            | x A B C D x |
     *                            | x E F G H x |
     *                            | x I J K L x |
     *                            | x M N O P x |
     *                            | x x x x x x |
     *                            +-------------+
     *
     *       où `x` est le rembourrage (padding = 1). Ce rang devrait envoyer
     *
     *        - la bordure [A B C D] au rang nord,
     *        - la bordure [M N O P] au rang sud,
     *        - la bordure [A E I M] au rang ouest et
     *        - la bordure [D H L P] au rang est.
     *
     *       Ce rang devrait aussi recevoir dans son rembourrage
     *
     *        - la bordure [A B C D] du rang sud,
     *        - la bordure [M N O P] du rang nord,
     *        - la bordure [A E I M] du rang est et
     *        - la bordure [D H L P] du rang ouest.
     *
     *       Après l'échange, le `grid` devrait avoir ces données dans son
     *       rembourrage provenant des voisins:
     *
     *                            +-------------+
     *                            | x m n o p x |
     *                            | d A B C D a |
     *                            | h E F G H e |
     *                            | l I J K L i |
     *                            | p M N O P m |
     *                            | x a b c d x |
     *                            +-------------+
     *
     *       Utilisez `grid_get_cell` pour obtenir un pointeur vers une cellule.
     */

    

fail_exit:
    return -1;
}

int heatsim_send_result(heatsim_t* heatsim, grid_t* grid) {
    assert(grid->padding == 0);

    /*
     * TODO: Envoyer les données (`data`) du `grid` résultant au rang 0. Le
     *       `grid` n'a aucun rembourage (padding = 0);
     */

fail_exit:
    return -1;
}

int heatsim_receive_results(heatsim_t* heatsim, cart2d_t* cart) {
    /*
     * TODO: Recevoir toutes les `grid` des autres rangs. Aucune `grid`
     *       n'a de rembourage (padding = 0).
     *
     *       Utilisez `cart2d_get_grid` pour obtenir la `grid` à une coordonnée
     *       qui va recevoir le contenue (`data`) d'un autre noeud.
     */

fail_exit:
    return -1;
}
