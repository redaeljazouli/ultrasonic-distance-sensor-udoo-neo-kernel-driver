//Inclusion des bibliothèques nécessaires
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

//Définition des informations du module
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver module");
MODULE_AUTHOR("RHM");
MODULE_VERSION("1.O");

//Déclaration des variables statiques
static int major;
static int irq_num;
static ktime_t start_time, end_time;
static s64 dur, resultat;
static bool mesure = false;

//Fonction qui gère les interruption sr le changement d'état du signal echo
static irqreturn_t echo_irq_handler(int irq, void *data) {
    //Si une mesure est en cours
    if (mesure) {
        //Enregistrement du temps de fin
        end_time = ktime_get();
        //Indiquer qu'aucune mesure n'est en cours
        mesure = false;
        //Calcul de la durée de l'impulsion
        dur = ktime_to_us(ktime_sub(end_time, start_time));
        //Calcul de la distance
        resultat = div_s64(dur, 58);
    } else {
        //Enregistrement du temps de début
        start_time = ktime_get();
        //Indiquer qu'une mesure est en cours
        mesure = true;
    }
    //Retourner le code de gestion d'interruption
    return IRQ_HANDLED;
}

//Fonction principale qui effectue les mesures et écrit les résultats
static ssize_t driver(struct file *fp, char __user *buf, size_t n, loff_t *of) {
    int ret,i=0;
    struct file *f;
    mm_segment_t fs;
    char buff[50];
    s64 sum=0,moyenne=0,et=0,variance=0,ecart_type=0;

    //Ouverture du fichier pour écrire les résultats
    f = filp_open("/home/udooer/Projet_final/resultat.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (!f) {
        printk(KERN_ALERT "filp_open failed.\n");
        return -1;
    }

    //Boucle pour effectuer 1000 mesures
    for(i=0;i<1000;i++){
        //Envoi du signal de déclenchement
	gpio_set_value(181, 1);
        //Pause de 10 microsecondes
        udelay(10);
        //Arrêt du signal de déclenchement
        gpio_set_value(181, 0);

        //Pause de 50 millisecondes
        msleep(50);
        //Affichage de la distance mesurée
        printk("La distance du test %d est de %lld\n" ,i, resultat);
        //Mise à jour de la somme des distances et de la somme des carrés des distances
        sum += resultat;
        et+= resultat*resultat;
    }

    //Calcul de la moyenne et de l'écart type
    moyenne = div_s64(sum,i);
    variance = div_s64((et-(div_s64(sum*sum,i))),(i-1));
    ecart_type = int_sqrt(variance);
    //Préparation du texte à écrire dans le fichier
    ret = snprintf(buff, 50, "La moyenne est : %lldcm\nL'ecart type est : %lld\n" ,moyenne,ecart_type);
    //Sauvegarde du segment de mémoire actuel
    fs = get_fs();
    //Passage au segment de mémoire du noyau
    set_fs(KERNEL_DS);
    //écriture du texte dans le fichier
    vfs_write(f, buff, ret, &f->f_pos);
    //Restauration du segment de mémoire précédent
    set_fs(fs);
    //Fermeture du fichier
    filp_close(f,NULL);
    //Retour de la fonction
    return 0;
}

//Définition de la structure des opérations de fichiers pour le module
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = driver,
};

//Fonction de sortie du module
static void kexit(void) {
    //Affichage d'un message pour signaler la sortie du module
    printk("Exit driver module\n");
    //Libération des GPIO
    gpio_free(180);
    gpio_free(181);
    //Libération de l'interruption
    free_irq(irq_num, NULL);
    //Désenregistrement du pilote de caractères
    unregister_chrdev(major, "dcat");
    //Retour de la fonction
    return;
}

//Fonction d'initialisation du module
static int kinit(void) {
    int entre, sortie;
    //Affichage d'un message pour signaler l'initialisation du module
    printk("Init driver module\n");

    //Demande d'allocation du GPIO pour le déclenchement (Trigger)
    sortie = gpio_request(181, "Trigger");
    if (sortie) {
        printk(KERN_ALERT "erreur allocation gpio 181 ");
        gpio_free(181);
        sortie = gpio_request(181, "Trigger");
        printk(KERN_ALERT "allocation gpio 181 done\n");
    }
    //Configuration du GPIO 181 en sortie
    gpio_direction_output(181, 0);

    //Demande d'allocation du GPIO pour l'écho (Echo)
    entre = gpio_request(180, "Echo");
    if (entre) {
        printk(KERN_ALERT "erreur allocation gpio 180 ");
        gpio_free(180);
        entre = gpio_request(180, "Echo");
        printk(KERN_ALERT "allocation gpio 180 done\n");
    }
    //Configuration du GPIO 180 en entrée
    gpio_direction_input(180);

    //Association de l'interruption au GPIO 180
    irq_num = gpio_to_irq(180);
    //Demande d'enregistrement de la fonction de gestion des interruptions
    if (request_irq(irq_num, echo_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "echo_irq", NULL)) {
        printk(KERN_ALERT "request_irq failed.\n");
        return -1;
    }

    //Enregistrement du pilote de caractères
    major = register_chrdev(0, "dcat", &fops);
    if (major < 0) {
        printk(KERN_ALERT "register_chrdev %d", major);
    }
    //Retour de la fonction
    return 0;
}

//Définition des fonctions d'initialisation et de sortie du module

module_init(kinit);
module_exit(kexit);
