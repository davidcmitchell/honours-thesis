/* Disclaimer: only works for graph with multiple edges where edges to the same node are adjacent to each other.
 Code written by David Mitchell based on instruction from Prof Wendy Myrvold University of Victoria */


#include <stdio.h>
#include <stdlib.h>

#define NMAX 512
#define DEG 3

struct Graph
{
    int adj_list[NMAX][DEG]; //a list of the vertices connected to each vertex
    int edge_list[NMAX][DEG]; //a list of the edges which connect vertices
    int num_edges[NMAX]; //the degree of a vertex
    int gap_list[NMAX][DEG]; //the faces associated with each edge
    int total_edges; //the number of edges in a graph
    int total_degree; //the number of degrees
    int num_faces; //the number of faces on a graph
    int num_nodes; //the number of vertices per graph
    int sg_num_nodes; //the number of vertices in the underlying graph
    int graph_number; //the graph number
};

int read_graph(struct Graph *graph, FILE *file); //reads in the graph, returns 0 if the end of file has been reached
void set_edges(struct Graph *graph); //sets the edge numbers
int num_multiedges(int vert, int nbr_vert, struct Graph *graph);
void set_multiedges(int num_me, int edge_count, int vert, int nbr_vert, struct Graph *graph);
void set_gap(struct Graph *graph);
void walk_face(int vert, int nbr_pos, int face_num, struct Graph * graph);
void bisect_graph(struct Graph *graph);
int get_pos(int find_val, int row[], int row_length);
void get_candidates(int v, int *num_candidates,int candidates[NMAX][3], struct Graph *graph);
int common_face(int v1, int v2, struct Graph *graph);
void add_chords(int level, struct Graph *graph, int * com_graphs);
void add_edge(int v1, int v2, int p1, int p2, struct Graph * graph);
void remove_edge (int v1, int v2, struct Graph *graph);

void print_for_cf(struct Graph *graph);
void print_graph(struct Graph *graph);
void print_edge_list(struct Graph *graph);
void print_gap(struct Graph *graph);
void print_faces(struct Graph * graph);

/***************************************************************************************/


int main(int argc, char* argv[])
{
    
    //clear the output streams
    fflush(stdout);
    int com_graphs = 0;
    
    //open the file containing the graph
    FILE *file;
    if (argc > 1)
    {
        file = fopen(argv[1], "r");
    }
    else
    {
        file = stdin;
    }
    
    //read the graph
    struct Graph graph;
    graph.graph_number = 1;
    while (read_graph(&graph,file) != 0)
    {
        set_edges(&graph);
        set_gap(&graph);
        bisect_graph(&graph);
        set_gap(&graph);
        add_chords(graph.sg_num_nodes, &graph, &com_graphs);
        graph.graph_number++;
        //printf("Completed graphs: %d\n",com_graphs);
        com_graphs = 0;
    }
    
}

/***************************************************************************************/

int read_graph(struct Graph *graph, FILE *file)

{
    int i;
    int j;
    
    if (!fscanf(file,"%d",&graph->num_nodes))
    {
        fprintf(stderr, "error reading in graph (error using scanf)\n");
        exit(1);
    }
    
    //if we've reached the end of the file return 0
    if (feof(file))
    {
        return 0;
    }
    
    //check the total number of nodes
    if (graph->num_nodes < 1)
    {
        fprintf(stderr, "error reading in graph\n");
        exit(1);
    }
    
    graph->total_degree = 0;
    
    // for each row use scanf to read the # of edges and insert them in the matrix
    for (i = 0; i < graph->num_nodes; i++)
    {
        if (!fscanf(file,"%d",&graph->num_edges[i]))
        {
            fprintf(stderr, "error reading in graph (error using scanf)\n");
            exit(1);
        }
        
        // check the edge numbers arent incorrect
        if (graph->num_edges[i] != DEG)
        {
            fprintf(stderr, "error reading in graph (graph must be %2d regular)\n",DEG);
            exit(1);
        }
        
        graph->total_degree += graph->num_edges[i];
        
        //read num_edges[i] nodes into the graph
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            if (!fscanf(file,"%d",&graph->adj_list[i][j]))
            {
                fprintf(stderr, "error reading in graph (error using scanf)\n");
                exit(1);
            }
            
            //input clearly can't exceed the number of nodes and can't be less than 0
            //as well, the graph contains no self loops
            if (graph->adj_list[i][j] > graph->num_nodes || graph->adj_list[i][j] < 0 || graph->adj_list[i][j] == i)
            {
                fprintf(stderr, "error reading in graph (incorrect node)\n");
                exit(1);
            }
        }
    }
    
    graph->total_edges = graph->total_degree/2;
    graph->num_faces = 2 + graph->total_edges - graph->num_nodes;
    graph->sg_num_nodes = graph->num_nodes;
    
    return 1;
}


/***************************************************************************************/

void set_edges(struct Graph *graph)

{
    int i;
    int j;
    int k;
    int num_me;
    int nbr_vert;
    int edge_count;
    
    //set all the edges to -1
    for (i = 0; i < graph->num_nodes; i++)
    {
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            graph->edge_list[i][j] = -1;
        }
    }
    
    //set all the edges
    edge_count = 2;
    for (i = 0; i < graph->num_nodes; i++)
    {
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            if (graph->edge_list[i][j] == -1) //the edge has not been set
            {
                //determine if there are multiple edges to the same vertex
                nbr_vert = graph->adj_list[i][j];
                num_me = num_multiedges(i,nbr_vert,graph);
                if (num_me > 1)
                {
                    set_multiedges(num_me,edge_count,i,nbr_vert,graph);
                    edge_count = edge_count+num_me;
                }
                else
                {
                    graph->edge_list[i][j] = edge_count;
                    
                    //change the edge on the home row of the neighbour vertex after finding it
                    k = 0;
                    while (graph->adj_list[nbr_vert][k] != i)
                    {
                        k++;
                    }
                    graph->edge_list[nbr_vert][k] = -edge_count;
                    edge_count++;
                }
            }
            
        }
    }
}

/***************************************************************************************/

void set_multiedges(int num_me, int edge_count, int vert, int nbr_vert, struct Graph * graph)

{
    int i;
    int j;
    
    //walk to the first instance of an edge to nbr_vert on the home row of vert
    i = 0;
    while (graph->adj_list[vert][i] == nbr_vert && i < graph->num_edges[vert])
    {
        i++;
    }
    while (graph->adj_list[vert][i % graph->num_edges[vert]] != nbr_vert)
    {
        i++;
    }
    
    //now label the edges
    for (j = 0; j < num_me; j++)
    {
        graph->edge_list[vert][(i + j) % graph->num_edges[vert]] = edge_count+j;
    }
    
    //repeat for the nbr vertex, but opposite order of edge numbers
    i = 0;
    while (graph->adj_list[nbr_vert][i] == vert && i < graph->num_edges[nbr_vert])
    {
        i++;
    }
    while (graph->adj_list[nbr_vert][i % graph->num_edges[nbr_vert]] != vert)
    {
        i++;
    }
    
    for (j = 0; j < num_me; j++)
    {
        graph->edge_list[nbr_vert][(i + j) % graph->num_edges[vert]] = -((edge_count-1)+num_me-j);
    }
    
}
/***************************************************************************************/


int num_multiedges(int vert, int nbr_vert, struct Graph *graph)

{
    int i;
    int count = 0;
    
    for (i = 0; i < graph->num_edges[vert]; i++)
    {
        if (graph->adj_list[vert][i] == nbr_vert)
        {
            count++;
        }
    }
    return count;
}


/***************************************************************************************/

void set_gap(struct Graph * graph)

{
    int i;
    int j;
    int face_num;
    
    //initialize the gap matrix to -1
    for (i = 0; i < graph->num_nodes; i++)
    {
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            graph->gap_list[i][j] = -1;
        }
    }
    
    face_num = 0;
    for (i = 0; i < graph->num_nodes; i++)
    {
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            //if the edge is not part of a face, walk the face and increment number of faces
            if (graph->gap_list[i][j] == -1)
            {
                walk_face(i, j, face_num, graph);
                face_num++;
            }
        }
    }
    
}

/***************************************************************************************/
void walk_face (int vert, int nbr_pos, int face_num, struct Graph *graph)

{
    int i;
    int first_edge = graph->edge_list[vert][nbr_pos];
    int next_edge = -1;
    int next_vert = -1;
    int curr_edge = first_edge;
    int curr_vert = graph->adj_list[vert][nbr_pos];
    
    while (next_edge != first_edge)
    {
        //move to the position following the last edge
        i = 0;
        while(graph->edge_list[curr_vert][i] != -curr_edge)
        {
            i++;
        }
        i++;
        
        //set the edge following the previous to be part of the face
        graph->gap_list[curr_vert][i % graph->num_edges[curr_vert]] = face_num;
        next_edge = graph->edge_list[curr_vert][i % graph->num_edges[curr_vert]];
        next_vert = graph->adj_list[curr_vert][i % graph->num_edges[curr_vert]];
        
        curr_edge = next_edge;
        curr_vert = next_vert;
    }
}

/***************************************************************************************/

void bisect_graph(struct Graph *graph)

{
    int i;
    int j;
    int nbr_vert;
    int new_vert;
    int edge_num;
    int pos;
    
    for (i = 0; i < graph->sg_num_nodes; i++)
    {
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            //if a vertex neighbour num is less than the num of vertices, it hasn't been bisected
            if (graph->adj_list[i][j] < graph->sg_num_nodes)
            {
                nbr_vert = graph->adj_list[i][j];
                edge_num = graph->edge_list[i][j];
                pos = get_pos(-edge_num, graph->edge_list[nbr_vert], graph->num_edges[nbr_vert]);
                
                new_vert = graph->num_nodes;
                graph->num_edges[new_vert] = 0;
                
                //set the home row of the vis vert to point to new vert
                graph->adj_list[i][j] = new_vert;
                
                //set the new vert to point to vis vert
                graph->adj_list[new_vert][0] = i;
                graph->total_edges++;
                graph->num_edges[new_vert]++;
                
                //set the home row of the nbr vert to point to new vert
                graph->adj_list[nbr_vert][pos] = new_vert;
                
                //set the new vert to point to nbr vert
                graph->adj_list[new_vert][1] = nbr_vert;
                graph->num_edges[new_vert]++;

                
                graph->num_nodes++;
            }
        }
    }
    set_edges(graph);
}

/***************************************************************************************/

void add_chords(int level, struct Graph *graph, int * com_graphs)
{
    int i;
    int v;
    int num_candidates;
    int candidates[NMAX][3];
    
    v = level;
    if (v == graph->num_nodes)
    {
        print_for_cf(graph);
        *com_graphs = *com_graphs + 1;
        return;
    }
    
    if (graph->num_edges[v] < DEG)
    {
        get_candidates(v, &num_candidates, candidates, graph);
        for (i = 0; i < num_candidates; i++)
        {
            add_edge(v, candidates[i][0],candidates[i][1],candidates[i][2], graph);
            add_chords(v+1, graph,com_graphs);
            remove_edge(v, candidates[i][0], graph);
        }
    }
    else
    {
        add_chords(v+1, graph, com_graphs);
    }
    
}

/***************************************************************************************/
void get_candidates(int v, int *num_candidates,int candidates[NMAX][3], struct Graph *graph) //
{
    
    
    /*hv (home vertex) pos and tv (to vertex) pos keep track of the positions where the new edge is to be added
    for the home vertex, the edge is added 1 before the vertex it initially leads to.
    for the to vertex, the edge is added 1 after the edge leading to it  */
    
    int i, j, k, c, hv_pos;
    c = 0;
    
    *num_candidates = 0;
    
    for (j = 0; j < DEG-1; j++)
    {
        int first_edge = graph->edge_list[v][j];
        int next_edge = -1;
        int next_vert = -1;
        int curr_edge = first_edge;
        int curr_vert = graph->adj_list[v][j];
        hv_pos = j;
        
        while (next_edge != first_edge)
        {
            //move to the position following the last edge
            i = 0;
            while(graph->edge_list[curr_vert][i] != -curr_edge)
            {
                i++;
            }
            i++;
            
            //if the vertex is only degree 2, make it a candidate
            if (graph->num_edges[curr_vert] == 2 && curr_vert != v)
            {
                candidates[c][0] = curr_vert;
                candidates[c][1] = hv_pos;
                
                //find the position of where the edge should be added
                k = 0;
                while (graph->edge_list[curr_vert][k] != -curr_edge)
                {
                    k++;
                }
                
                candidates[c][2] = k;
                c++;
                *num_candidates = *num_candidates + 1;
            }
            
            
            next_edge = graph->edge_list[curr_vert][i % graph->num_edges[curr_vert]];
            next_vert = graph->adj_list[curr_vert][i % graph->num_edges[curr_vert]];
            
            curr_edge = next_edge;
            curr_vert = next_vert;
        }
    }
    
}

/***************************************************************************************/
void add_edge(int v1, int v2, int p1, int p2, struct Graph * graph)

{
    int j;
    
    //increase the num of edges of v1 and v2
    graph->num_edges[v1]++;
    graph->num_edges[v2]++;
    
    //find the position for the new edge on v1
    
    for (j = graph->num_edges[v1] - 1; j > p1; j--)
    {
        graph->adj_list[v1][j] = graph->adj_list[v1][j-1];
        graph->edge_list[v1][j] = graph->edge_list[v1][j-1];
    }
    
    graph->adj_list[v1][p1] = v2;
    graph->edge_list[v1][p1] = graph->total_edges+2;
    
    //find the position for the new edge on v2
    
    for (j = graph->num_edges[v2] - 1; j > p2+1; j--)
    {
        graph->adj_list[v2][j] = graph->adj_list[v2][j-1];
        graph->edge_list[v2][j] = graph->edge_list[v2][j-1];
    }
    
    graph->adj_list[v2][p2+1] = v1;
    graph->edge_list[v2][p2+1] = -(graph->total_edges+2);
    
    graph->total_edges++;
    graph->num_faces++;
    
    set_gap(graph);
    
}

/***************************************************************************************/

void remove_edge (int v1, int v2, struct Graph *graph)

{
    int i;
    int j;
    int k;
    
    //find the position of v2
    i = 0;
    while (graph->adj_list[v1][i] != v2)
    {
        i++;
    }
    
    //copy the array but bypass pos of v2
    k = 0;
    for (j = 0; j < graph->num_edges[v1]; j++)
    {
        if (j != i)
        {
            graph->adj_list[v1][k] = graph->adj_list[v1][j];
            graph->edge_list[v1][k] = graph->edge_list[v1][j];
            k++;
        }
    }
    graph->num_edges[v1]--;
    
    //do the same for v2
    i = 0;
    while (graph->adj_list[v2][i] != v1)
    {
        i++;
    }
    
    //copy the array but bypass pos of v1
    k = 0;
    for (j = 0; j < graph->num_edges[v2]; j++)
    {
        if (j != i)
        {
            graph->adj_list[v2][k] = graph->adj_list[v2][j];
            graph->edge_list[v2][k] = graph->edge_list[v2][j];
            k++;
        }
    }
    graph->num_edges[v2]--;
    
    graph->total_edges--;
    graph->num_faces--;
    
    set_gap(graph);
}

/***************************************************************************************/

int common_face(int v1, int v2, struct Graph *graph)

{
    int i;
    int j;
    for (i = 0; i < graph->num_edges[v1]; i++)
    {
        for (j = 0; j < graph->num_edges[v2]; j++)
        {
            if (graph->gap_list[v1][i] == graph->gap_list[v2][j])
            {
                return graph->gap_list[v1][i];
            }
            
        }
    }
    return -1;
}


/***************************************************************************************/

int get_pos(int find_val, int row[], int row_length)

{
    int i;
    for (i = 0; i < row_length; i++)
    {
        if (row[i] == find_val)
        {
            return i;
        }
    }
    return -1;
}
/***************************************************************************************/

void print_graph(struct Graph *graph)

{
    int i;
    int j;
    
    printf("%d ",graph->num_nodes);
    for (i = 0; i < graph->num_nodes; i++)
    {
        printf("%d ", graph->num_edges[i]);
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            printf("%d ", graph->adj_list[i][j]);
        }
    }
    printf("\n");
}

/***************************************************************************************/

void print_for_cf(struct Graph *graph)

{
    int i;
    int j;
    printf("%d ", graph->graph_number);
    printf("%d ", graph->num_nodes);
    for (i = 0; i < graph->num_nodes; i++)
    {
        printf ("%d ", graph->num_edges[i]);
        
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            printf("%d %d ", graph->adj_list[i][j], graph->edge_list[i][j]);
        }
        
    }
    printf("\n");
}




/***************************************************************************************/

void print_edge_list(struct Graph *graph)

{
    printf("Printing edge list\n");
    int i;
    int j;
    for (i = 0; i < graph->num_nodes; i++)
    {
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            printf("%d ", graph->edge_list[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

/***************************************************************************************/

void print_gap(struct Graph *graph)

{
    printf("Printing gap list\n");
    int i;
    int j;
    for (i = 0; i < graph->num_nodes; i++)
    {
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            printf("%d ", graph->gap_list[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

/***************************************************************************************/

void print_faces(struct Graph * graph)

{
    int i;
    int j;
    int k;
    int l;
    int first_edge;
    int next_edge;
    int next_vert;
    int curr_edge;
    int curr_vert;
    
    printf("Printing faces\n");
    for (i = 0; i < graph->num_faces; i++)
    {
        printf("Printing face #%d:\n", i);
        
        j = 0;
        k = 0;
        while (graph->gap_list[j][k] != i)
        {
            if (k < graph->num_edges[j])
            {
                k++;
            }
            else
            {
                j++;
                k = 0;
            }
        }
        
        first_edge = graph->edge_list[j][k];
        next_edge = -1;
        next_vert = -1;
        curr_edge = first_edge;
        curr_vert = graph->adj_list[j][k];
        
        while (next_edge != first_edge)
        {
            //move to the position following the last edge
            l = 0;
            while(graph->edge_list[curr_vert][l] != -curr_edge)
            {
                l++;
            }
            l++;
            
            next_edge = graph->edge_list[curr_vert][l % graph->num_edges[curr_vert]];
            next_vert = graph->adj_list[curr_vert][l % graph->num_edges[curr_vert]];
            
            printf("Vertex %d to vertex %d edge number %d.\n", curr_vert, next_vert, curr_edge);
            
            curr_edge = next_edge;
            curr_vert = next_vert;
        }
        printf("\n\n");
    }
    printf("\n");
}


